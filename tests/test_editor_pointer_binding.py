"""Synthetic fresh source/start evidence; no installed target modules imported."""
from contextlib import contextmanager
import copy
import json
import os
from pathlib import Path
import subprocess
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import editor_pointer_binding as binder
import editor_pointer_parent as parent
import test_editor_pointer_parent as parent_tests


class BindingTests(unittest.TestCase):
    ref = parent_tests.ParentBindingTests.ref
    jsonref = parent_tests.ParentBindingTests.jsonref
    pin = parent_tests.ParentBindingTests.pin

    def setUp(self):
        parent_tests.ParentBindingTests.setUp(self)
        self.events = []; self.phase = 'READY'; self.guest_error = False; self.readiness_error = False
        self.window = Path(self.freeze['newWindow']); self.session = Path(self.modules['capture_runner']).parent
        sources = self.freeze['sources']
        for path in list(sources): sources[path] = self.ref(path, b'# FAKE HOST source\n')
        extras = {n: str(self.session / (n + '.py')) for n in binder.EXTRA}
        required = set(extras.values()) | {str(parent.TOOLS / (n + '.py')) for n in binder.LIFECYCLE}
        required |= {str(self.session / n) for n in ('SOURCE-PROVENANCE.json', 'publish_mouse_observation.py', 'clock_runtime.py')}
        for path in required: sources[path] = self.ref(path, b'# FAKE HOST source\n')
        ready_ref = self.jsonref(self.session / 'ACTUAL-FAKE-HOST-READINESS.json', {
            'state': 'ROOT_ACTUALLY_VERIFIED_HUMAN_APPROVED_DEVBENCH_READY', 'acceptedByRoot': True,
            'authorizationRef': self.jsonref('/fixture/HOST-auth', {'FAKE_HUMAN': True}),
            'restartIntentRef': self.jsonref('/fixture/HOST-restart-intent', {'FAKE_INTENT': True}),
            'restartResultRef': self.jsonref('/fixture/HOST-restart-result', {'FAKE_RESULT': True}),
            'observed': {'job': {'pid': '555'}}})
        self.freeze.update(extraModulePaths=extras, actualApprovedServiceReadinessRef=ready_ref)
        freeze_ref = self.jsonref(self.session / 'SOURCE-FREEZE.json', self.freeze)
        before = self.ref(self.window / 'before-canonicalProfile.bin', b'HOST profile')
        active = self.ref(binder.HARNESS / 'runtime/amiberry/Configurations/A1200-030-DEV.uae', b'HOST profile-PRIVATE')
        prep_ref = self.jsonref('/fixture/PREPARATION.json', {
            'state': 'PRIVATE_LOCAL_CANDIDATE_VOLUME_AND_ADDITIVE_PROFILE_SUFFIX_PREPARED_NOT_ADMITTED_OR_MOUNTED',
            'targetOperations': 0, 'canonicalProfileWrites': 0,
            'originalCandidate': self.b['blankBinding']['candidate'], 'privateCandidate': self.b['mountedCandidate'],
            'guestCandidate': 'PTCapture:PT24GEdit', 'appendASCII': '-PRIVATE',
            'proposedProfile': {'bytes': 20, 'sha256': parent.digest(b'HOST profile-PRIVATE')[1]}})
        self.owner = self.b['ownerRunID']
        owned = {'pid': 999999, 'argv': ['999999', '999999', 'FAKE_VM'], 'socket_inode': 123}
        self.record = {'state': binder.BIND, 'acceptedByRoot': True, 'windowDirectory': str(self.window),
            'sourceFreezeRef': freeze_ref, 'limits': {'whole': 60},
            'allowedActions': {'guestCPUAdmissions': 1, 'bindingWrites': 2, 'applicationLaunches': 0,
                               'input': 0, 'serviceChanges': 0, 'guardReleases': 0},
            'reservationRef': self.jsonref(self.window / 'reserve-result.json', {
                'state': 'FRESH_OWN_RESERVATION_NO_LAUNCH_OR_INPUT', 'run_id': self.owner,
                'window': str(self.window), 'target': 'amiberry-030', 'serviceReadinessRef': ready_ref}),
            'startupResultRef': self.jsonref(self.window / 'startup-result.json', {'returncode': 0}),
            'ownedProcessRef': self.jsonref(self.window / 'ROOT_CURRENT_STARTED_PROCESS.json', owned),
            'serviceProcessRef': self.jsonref(self.window / 'ROOT_CURRENT_DEVBENCH_PROCESS.json',
                {'pid': 555, 'argv': ['555', '555', 'FAKE_SERVICE']}),
            'nativeViewRef': self.jsonref(self.window / 'ROOT_NATIVE_WORKBENCH_VIEW.json',
                {'actuallyViewed': True, 'pid': 999999, 'applicationVisible': False}),
            'originalProfileRef': before, 'activeProfileRef': active,
            'preservationBeforeRef': self.jsonref(self.window / 'fresh-preservation-before.json', {'FAKE_BEFORE': True}),
            'preparationRef': prep_ref, 'geometryProofRef': self.b['nativeGeometryProofRef'],
            'probeNativeRef': self.b['passiveProbe']['nativeCandidate'],
            'probeSourceRef': self.b['passiveProbe']['nativeSource'],
            'probeCompileRef': self.b['passiveProbe']['compileReceipt'], 'runLabel': self.b['runLabel']}
        self.saved = {}
        def save(path, value):
            self.assertNotIn(str(path), self.saved); self.saved[str(path)] = copy.deepcopy(value)
            return self.jsonref(path, value)
        for target, replacement in [('parent.pinned', self.pin), ('binder.directory_ref', lambda p: {}),
                                    ('binder.profile_append', lambda *a: b'-PRIVATE'), ('binder.save', save)]:
            module, name = target.split('.')
            p = patch.object(parent if module == 'parent' else binder, name, replacement)
            p.start(); self.addCleanup(p.stop)
        p = patch.object(binder.os.path, 'lexists', side_effect=lambda path: str(path) in self.saved)
        p.start(); self.addCleanup(p.stop)
        original_validate = parent.validate_binding
        p = patch.object(parent, 'validate_binding', side_effect=lambda b, **kw: original_validate(b, pin=self.pin, **kw))
        p.start(); self.addCleanup(p.stop)
        @contextmanager
        def command_lock(path): yield 42
        p = patch.object(parent, 'command_lock', command_lock); p.start(); self.addCleanup(p.stop)
        p = patch.dict(os.environ, {}, clear=True); p.start(); self.addCleanup(p.stop)
        outer = self
        class Guard:
            def __init__(self, h): pass
            def status(self): return {'phase': outer.phase, 'holds': [{}] if outer.phase == 'HOLD' else [], 'inflight': None}
            @contextmanager
            def _locked(self): yield None, {}
            def _owner(self, raw, lease, target): outer.assertEqual(lease['run_id'], outer.owner)
        @contextmanager
        def operation(*a):
            self.events.append('operation-enter')
            try: yield {'FAKE_FRAME': True}
            except BaseException: self.events.append('operation-error'); raise
            else: self.events.append('operation-return')
        class API:
            def __init__(self, blank, frame): outer.assertTrue(frame['FAKE_FRAME'])
            def owner(self, owner): outer.assertEqual(owner, outer.owner); outer.events.append('owner')
        def guest(*a):
            self.events.append('guest')
            if self.guest_error: raise RuntimeError('FAKE original Guest failure')
        def ready(*a):
            self.events.append('readiness')
            if self.readiness_error: raise RuntimeError('FAKE current readiness refusal')
        @contextmanager
        def locks(): self.events.append('both-locks'); yield
        self.extras = {'service_readiness': SimpleNamespace(validate_records=ready, require_human_approved_service=ready),
            'fresh_preservation': SimpleNamespace(verify_current=lambda before: {'FAKE_VERIFIED': True}),
            'canonical_profile_session_files': SimpleNamespace(locks=locks)}
        self.d = SimpleNamespace(extras=self.extras, guard=Guard, lease_input=lambda *a: {'run_id': self.owner},
            operation=operation, original_api=API, guest=guest,
            blank_scope=SimpleNamespace(ALLOWED={'fixture': 1}, LIMITS={'whole': 300}, HISTORY_BYTES=182388),
            observations=SimpleNamespace(command=lambda *a: '555 555 FAKE_SERVICE'))

    def bind(self): return binder.execute(self.record, self.d)

    def test_actual_start_composes_guarded_guest_then_stage_plan_without_fake_endpoint(self):
        result = self.bind(); value = self.saved[result['path']]
        self.assertEqual(value['state'], parent.STAGE_PLAN_STATE)
        self.assertNotIn('stagedCandidate', value['passiveProbe']); self.assertNotIn('namespaceStageRef', value['passiveProbe'])
        self.assertEqual(self.events, ['readiness', 'operation-enter', 'owner', 'guest', 'owner', 'operation-return'])
        self.assertEqual(value['blankBinding']['preservation'], {'FAKE_VERIFIED': True})

    def test_failed_guest_keeps_intent_without_parent_binding_or_replay(self):
        self.guest_error = True
        with self.assertRaises(RuntimeError): self.bind()
        self.assertIn(str(self.window / 'mounted-guest-admission-intent.json'), self.saved)
        self.assertNotIn(str(self.window / 'mouse-parent-binding.json'), self.saved)
        with self.assertRaises(RuntimeError): self.bind()
        self.assertEqual(self.events.count('guest'), 1)

    def test_held_owner_or_bad_ready_refuses_before_guest(self):
        self.phase = 'HOLD'
        with self.assertRaises(RuntimeError): self.bind()
        self.assertNotIn('guest', self.events)
        self.phase = 'READY'; self.readiness_error = True
        with self.assertRaises(RuntimeError): self.bind()
        self.assertNotIn('guest', self.events)

    def test_unviewed_native_or_failed_startup_cannot_create_binding(self):
        for key, value in [('nativeViewRef', {'actuallyViewed': False, 'pid': 999999, 'applicationVisible': False}),
                           ('startupResultRef', {'returncode': 1})]:
            original = self.record[key]; self.record[key] = self.jsonref(original['path'], value)
            with self.assertRaises(RuntimeError): self.bind()
            self.record[key] = original
            self.bodies[original['path']] = json.dumps({'actuallyViewed': True, 'pid': 999999, 'applicationVisible': False}
                if key == 'nativeViewRef' else {'returncode': 0}).encode()
        self.assertNotIn('guest', self.events)

    def test_service_epoch_or_active_profile_change_refuses(self):
        original = self.record['serviceProcessRef']
        self.record['serviceProcessRef'] = self.jsonref(original['path'], {'pid': 556, 'argv': ['556', '556', 'OTHER']})
        with self.assertRaises(RuntimeError): self.bind()
        self.assertFalse(self.saved)

    def test_extra_target_scope_or_inherited_frame_refuses(self):
        self.record['allowedActions']['serviceChanges'] = 1
        with self.assertRaises(RuntimeError): self.bind()
        self.record['allowedActions']['serviceChanges'] = 0
        with patch.dict(os.environ, {'AMIGA_LAUNCH_INFLIGHT': 'FAKE'}):
            with self.assertRaises(RuntimeError): self.bind()
        self.assertNotIn('guest', self.events)

    def test_missing_full_lifecycle_source_refuses_before_original_module_load(self):
        path = str(parent.TOOLS / 'editor_capture_settlement.py')
        del self.freeze['sources'][path]
        self.record['sourceFreezeRef'] = self.jsonref(self.record['sourceFreezeRef']['path'], self.freeze)
        with self.assertRaises(RuntimeError): self.bind()
        self.assertNotIn('guest', self.events)

    def test_source_freeze_needs_actual_ready_under_both_original_locks(self):
        plan = dict(self.freeze, state=binder.PLAN, acceptedByRoot=True,
                    serviceReadinessRef=self.freeze['actualApprovedServiceReadinessRef'])
        result = binder.freeze_sources(plan, self.extras)
        self.assertEqual(self.events, ['both-locks', 'readiness'])
        self.assertEqual(self.saved[result['path']]['state'], parent.FROZEN_STATE)
        self.assertFalse(self.saved[result['path']]['runtimeAdmission'])

    def test_readiness_refusal_or_existing_window_cannot_publish_source_freeze(self):
        plan = dict(self.freeze, state=binder.PLAN, acceptedByRoot=True,
                    serviceReadinessRef=self.freeze['actualApprovedServiceReadinessRef'])
        self.readiness_error = True
        with self.assertRaises(RuntimeError): binder.freeze_sources(plan, self.extras)
        self.assertFalse(self.saved)
        with patch.object(binder.os.path, 'lexists', return_value=True):
            with self.assertRaises(RuntimeError): binder.freeze_sources(plan, self.extras)

    def test_shadowed_selected_source_refuses_before_import(self):
        name = 'launcher_guard'; path = self.modules[name]
        with patch.object(binder.importlib.util, 'find_spec', return_value=SimpleNamespace(origin='/wrong')), \
             patch.object(binder.importlib, 'import_module') as imported:
            with self.assertRaises(RuntimeError): binder.checked_module(name, path, self.freeze['sources'])
            imported.assert_not_called()

    def saved_helper_fixture(self):
        path = self.freeze['extraModulePaths']['canonical_profile_session_files']
        return SimpleNamespace(__file__=path, I=binder.HARNESS,
            ROLES={'canonicalProfile': binder.HARNESS / 'runtime/amiberry/Configurations/A1200-030-DEV.uae',
                   'emulatorState': binder.HARNESS / 'runtime/emulator.json',
                   'bridgeEnabled': binder.HARNESS / 'runtime/Dev/Tools/bridge-enabled'}, APPEND=b'OLD TEMP VOLUME')

    def test_saved_file_helper_uses_exact_new_mount_for_patch_and_restore(self):
        f = self.saved_helper_fixture()
        with patch.object(binder, 'SAVED_FILE_HELPER', parent.digest(self.pin(self.freeze['sources'][f.__file__]))):
            binder.configure_saved_files(f, self.freeze['sources'], self.record['preparationRef'], self.record['originalProfileRef'])
        self.assertEqual(f.APPEND, b'-PRIVATE'); self.assertEqual(f.SESSION, parent.digest(b'HOST profile-PRIVATE'))

    def test_changed_proposed_profile_never_reconfigures_original_helper(self):
        f = self.saved_helper_fixture(); prep = json.loads(self.pin(self.record['preparationRef']))
        prep['proposedProfile']['bytes'] += 1; changed = self.jsonref('/fixture/changed-prep', prep)
        with patch.object(binder, 'SAVED_FILE_HELPER', parent.digest(self.pin(self.freeze['sources'][f.__file__]))):
            with self.assertRaises(RuntimeError): binder.configure_saved_files(f, self.freeze['sources'], changed, self.record['originalProfileRef'])
        self.assertEqual(f.APPEND, b'OLD TEMP VOLUME')

    def test_actual_alternate_runtime_refuses_before_binding_imports(self):
        child = subprocess.run(['/usr/bin/python3', '-B', binder.__file__, '--help'], capture_output=True, text=True, timeout=20)
        self.assertNotEqual(child.returncode, 0); self.assertEqual(child.stdout, '')
        self.assertIn('Original harness clock required before binding imports', child.stderr)
        self.assertNotIn('TypeError', child.stderr)


if __name__ == '__main__': unittest.main()
