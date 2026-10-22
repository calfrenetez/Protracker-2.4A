"""Real private inputs, synthetic original owner/FRAME and IO; no guest."""
import asyncio
import copy
from contextlib import contextmanager
import json
import os
from pathlib import Path
import stat
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import editor_readonly_probe as ro
import editor_pointer_probe as legacy
import editor_pointer_parent as parent
import editor_probe_namespace as namespace
import editor_probe_volume as volume
import test_editor_probe_volume as preparation_tests
from test_editor_pointer_probe import stdout


class ReadonlyProbeTests(unittest.TestCase):
    prepare = preparation_tests.ProbeVolumeTests.prepare

    def setUp(self):
        preparation_tests.ProbeVolumeTests.setUp(self)
        for name in ('NATIVE', 'SOURCE', 'EDITOR'):
            p = patch.object(legacy, name, getattr(volume, name)); p.start(); self.addCleanup(p.stop)
        self.wtemp = tempfile.TemporaryDirectory(prefix='pt24g-readonly-window-HOST-', dir='/private/tmp')
        self.addCleanup(self.wtemp.cleanup); self.window = Path(self.wtemp.name)
        self.label = 'blank-capture-readonly-probe-host-r1'
        self.out = self.window / self.label / 'mouse-phase/passive-pointer'
        self.out.parent.mkdir(parents=True)
        prep_ref = self.prepare(); prep = volume.verify(prep_ref)
        sources = {}; modules = {}
        local = {'capture_runner', 'capture_case_mounted', 'first_mouse_api', 'capture_support', 'root_observation'}
        shared = {'launcher_guard', 'launcher_access', 'bridge_checks', 'shared_guest', 'emulator'}
        for name in ro.MODULES:
            if name in local:
                path = self.root / (name + '.py'); path.write_bytes(b'# HOST original API stand-in; never imported\n')
            elif name in shared: path = parent.HARNESS / 'scripts' / (name + '.py')
            elif name.startswith('amiberry_mcp.'):
                path = parent.HARNESS / '.venv/lib/python3.14/site-packages/amiberry_mcp' / (name.split('.')[-1] + '.py')
            else: path = parent.TOOLS / (name + '.py')
            modules[name] = str(path); sources[str(path)] = parent.read(path)[1]
        extras = {}
        for name in ('fresh_preservation', 'canonical_profile_session_files', 'service_readiness'):
            path = self.root / (name + '.py'); path.write_bytes(b'# HOST helper stand-in; never imported\n')
            extras[name] = str(path); sources[str(path)] = parent.read(path)[1]
        sources[str(Path(parent.__file__))] = parent.read(parent.__file__)[1]
        self.freeze = {'state': ro.FROZEN_STATE, 'newWindow': str(self.window), 'modulePaths': modules,
                       'sources': sources, 'extraModulePaths': extras, 'readonlyPreparationRef': prep_ref}
        freeze_ref = volume.save(self.root / 'SOURCE-FREEZE.json', self.freeze)
        self.active = self.original + prep['appendASCII'].encode('ascii')
        self.active_ref = {'path': str(parent.HARNESS / 'runtime/amiberry/Configurations/A1200-030-DEV.uae'),
                           'bytes': len(self.active), 'sha256': parent.digest(self.active)[1],
                           'fullFacts': {'HOST_SYNTHETIC_PROFILE_NO_SHARED_WRITE': True}}
        self.b = {'state': ro.INPUT_STATE, 'acceptedByRoot': True, 'target': 'amiberry-030',
            'allowedActions': dict(ro.ALLOWED), 'limits': dict(ro.LIMITS), 'ownerRunID': 'fixture-owner',
            'sourceFreezeRef': freeze_ref, 'sourcePins': sources, 'windowDirectory': str(self.window),
            'runLabel': self.label, 'guestCandidate': 'PTProbe:PTEditorPointerV2',
            'readonlyPreparationRef': prep_ref, 'nativeCandidate': self.refs['nativeCandidate'],
            'nativeSource': self.refs['nativeSource'], 'compileReceipt': self.refs['compileReceipt'],
            'stagedCandidate': prep['candidate'], 'editorCandidate': json.loads(parent.pinned(
                self.refs['capturePreparation']))['privateCandidate'],
            'capturePreparationRef': self.refs['capturePreparation'],
            'originalProfileBeforeimage': self.refs['originalProfile'], 'activeProfile': self.active_ref}
        self.alive = True; self.now = 100.; self.events = []; self.option = None; self.stopped = False
        outer = self
        class API:
            def clock(self): return outer.now
            def owner(self, owner):
                if not outer.alive or owner != 'fixture-owner': raise RuntimeError('HOST owner loss')
            def pinned(self, ref):
                if ref['path'] == outer.active_ref['path']:
                    if ref != outer.active_ref: raise RuntimeError('HOST profile pin changed')
                    return outer.active
                return parent.pinned(ref)
            read = staticmethod(parent.read)
            def save(self, path, value): volume.save(path, value)
            def stopped_owner(self, owner, mode):
                if not outer.stopped or mode not in ('normal', 'failed') or owner != 'fixture-owner':
                    raise RuntimeError('HOST stopped owner refusal')
                outer.events.append(('stopped', mode))
        class IO:
            async def readback(self, guest, path):
                outer.events.append(('readback', guest)); path.write_bytes(b'wrong' if outer.option == 'wrongbytes' else outer.native)
                if outer.option == 'owner-loss': outer.alive = False
                if outer.option == 'profile-drift': outer.active = b'changed after readback'
                if outer.option == 'deadline': outer.now += 21
                if outer.option == 'cancel': raise asyncio.CancelledError()
            async def snapshot(self, guest, out):
                outer.events.append(('execute', guest)); return stdout()
        self.api = API(); self.io = IO()
        self.probe = ro.ReadonlyEditorPointerProbe(self.b, self.api, io=self.io)

    def run_probe(self): return asyncio.run(self.probe.run(self.out, 200.))

    def test_exact_mounted_input_uses_one_complete_readback_before_one_execution(self):
        result = self.run_probe()
        self.assertEqual(self.events, [('readback', 'PTProbe:PTEditorPointerV2'), ('execute', 'PTProbe:PTEditorPointerV2')])
        self.assertEqual(result['state'], 'PASSIVE_EDITOR_V2_DATA_RECORDED')
        self.assertFalse(result['inputAuthorized']); self.assertFalse(result['hotspotMappingAccepted'])
        self.assertEqual((result['readbackAttempts'], result['executionAttempts']), (1, 1))
        self.assertEqual(parent.pinned(self.b['stagedCandidate']), self.native)
        with self.assertRaisesRegex(RuntimeError, 'Never replay'): self.run_probe()

    def test_wrong_guest_bytes_owner_loss_or_profile_drift_never_executes(self):
        # Each subtest constructs its own fresh one-shot carrier and output.
        for option in ('wrongbytes', 'owner-loss', 'profile-drift', 'deadline'):
            with self.subTest(option=option):
                self.option = option; self.alive = True; self.now = 100.
                prep = volume.verify(self.b['readonlyPreparationRef'])
                self.active = self.original + prep['appendASCII'].encode('ascii')
                out = self.window / ('blank-capture-' + option + '-r1') / 'mouse-phase/passive-pointer'
                out.parent.mkdir(parents=True); b = dict(self.b, runLabel=out.parent.parent.name)
                p = ro.ReadonlyEditorPointerProbe(b, self.api, io=self.io); self.events = []
                with self.assertRaises(RuntimeError): asyncio.run(p.run(out, 200.))
                result = json.loads((out / 'result.json').read_text())
                self.assertEqual((result['readbackAttempts'], result['executionAttempts']), (1, 0))
                self.assertEqual(self.events, [('readback', 'PTProbe:PTEditorPointerV2')])

    def test_cancellation_retains_result_and_never_replays(self):
        self.option = 'cancel'
        with self.assertRaises(asyncio.CancelledError): self.run_probe()
        result = json.loads((self.out / 'result.json').read_text())
        self.assertEqual(result['executionAttempts'], 0)
        with self.assertRaisesRegex(RuntimeError, 'Never replay'): self.run_probe()

    def test_initial_profile_mismatch_retains_zero_dispatch_receipt(self):
        self.active = self.original
        with self.assertRaisesRegex(RuntimeError, 'paired read-only profile'): self.run_probe()
        result = json.loads((self.out / 'result.json').read_text())
        self.assertEqual((result['readbackAttempts'], result['executionAttempts']), (0, 0))
        self.assertFalse(self.events)

    def test_tracking_flag_change_strictly_refuses_before_io(self):
        if not hasattr(os, 'chflags') or not hasattr(stat, 'UF_TRACKED'): self.skipTest('Darwin metadata')
        path = Path(self.b['stagedCandidate']['path']); os.chflags(path, path.stat().st_flags | stat.UF_TRACKED)
        with self.assertRaises(RuntimeError): self.run_probe()
        self.assertFalse(self.events)
        self.assertEqual(json.loads((self.out / 'result.json').read_text())['readbackAttempts'], 0)

    def test_prepared_directory_time_change_refuses_before_io(self):
        st = self.destination.stat()
        os.utime(self.destination, ns=(st.st_atime_ns, st.st_mtime_ns + 1000000))
        with self.assertRaises(RuntimeError): self.run_probe()
        self.assertFalse(self.events)

    def test_input_validation_itself_cannot_extend_twenty_second_budget(self):
        original = self.api.owner; calls = []
        def slow(owner):
            original(owner); calls.append(owner)
            if len(calls) == 3: self.now += 21
        self.api.owner = slow
        with self.assertRaisesRegex(RuntimeError, 'deadline exhausted during input validation'): self.run_probe()
        self.assertFalse(self.events)
        self.assertEqual(json.loads((self.out / 'result.json').read_text())['readbackAttempts'], 0)

    def test_legacy_freeze_cannot_authorize_readonly_input(self):
        f = copy.deepcopy(self.freeze); f['state'] = parent.FROZEN_STATE
        path = Path(self.b['sourceFreezeRef']['path']); path.write_text(json.dumps(f))
        self.b['sourceFreezeRef'] = parent.read(path)[1]
        with self.assertRaisesRegex(RuntimeError, 'distinct read-only source freeze'): ro.prepared_input(self.b)

    def test_missing_or_shadowed_selected_module_refuses(self):
        for variant in ('missing', 'shadowed'):
            f = copy.deepcopy(self.freeze)
            if variant == 'missing': del f['modulePaths']['editor_readonly_probe']
            else: f['modulePaths']['editor_readonly_probe'] = '/wrong/editor_readonly_probe.py'
            path = Path(self.b['sourceFreezeRef']['path']); path.write_text(json.dumps(f))
            b = dict(self.b, sourceFreezeRef=parent.read(path)[1])
            with self.assertRaises(RuntimeError): ro.prepared_input(b)
        self.assertFalse(self.events)

    def test_preparation_or_old_namespace_cannot_replace_new_admission(self):
        for key, value in (('state', volume.STATE), ('acceptedByRoot', False),
                           ('target', 'real-a1200'), ('guestCandidate', 'Dev:Tests/probe')):
            b = dict(self.b); b[key] = value
            with self.subTest(key=key), self.assertRaises(RuntimeError): ro.prepared_input(b)
        b = dict(self.b, namespaceStageRef={'historical': True})
        with self.assertRaises(RuntimeError): ro.prepared_input(b)

    def test_other_evidence_endpoint_consumes_attempt_without_io(self):
        with self.assertRaises(RuntimeError): asyncio.run(self.probe.run(self.window / 'wrong', 200.))
        with self.assertRaisesRegex(RuntimeError, 'Never replay'): self.run_probe()
        self.assertFalse(self.events)

    def test_existing_executable_parent_still_rejects_child_admission(self):
        with self.assertRaisesRegex(RuntimeError, 'Distinct exact parent admission'):
            parent.validate_binding(self.b)

    def stopped_lock(self):
        root = self.root / 'HOST-harness'; (root / 'runtime').mkdir(parents=True)
        lock = root / 'runtime/test.lock'; lock.touch()
        p = patch.object(namespace, 'HARNESS', root); p.start(); self.addCleanup(p.stop)
        self.stopped = True
        return parent.command_lock(lock)

    def test_failed_stopped_retention_uses_same_lock_without_removal(self):
        with self.stopped_lock() as fd:
            ref = ro.preserve_stopped(self.b, self.window / 'preserved-readonly-probe-input',
                                      self.api, mode='failed', lock_fd=fd)
            # Profile restoration does not grant runtime authority and is not
            # required to keep an immutable input's stopped retention facts.
            self.active = self.original
            facts = ro.release_facts(self.b, ref, self.api, mode='failed', lock_fd=fd)
            self.assertFalse(facts['readonlyProbeInput']['guardReleaseClaimed'])
            self.assertEqual(os.fstat(fd).st_ino, (self.root / 'HOST-harness/runtime/test.lock').stat().st_ino)
            self.assertEqual(parent.pinned(self.b['stagedCandidate']), self.native)
            self.assertEqual([p.name for p in self.destination.iterdir()], ['PTEditorPointerV2'])
            self.assertEqual(facts['readonlyProbeInput']['facts']['probeFilesRemoved'], 0)
            with self.assertRaises(RuntimeError):
                ro.preserve_stopped(self.b, self.window / 'preserved-readonly-probe-input', self.api, mode='failed', lock_fd=fd)

    def test_normal_stopped_input_retention_is_not_runtime_or_release_authority(self):
        with self.stopped_lock() as fd:
            ref = ro.preserve_stopped(self.b, self.window / 'preserved-readonly-probe-input', self.api, mode='normal', lock_fd=fd)
            facts = ro.release_facts(self.b, ref, self.api, mode='normal', lock_fd=fd)
        self.assertFalse(facts['readonlyProbeInput']['runtimeAdmission'])
        self.assertFalse(facts['readonlyProbeInput']['guardReleaseClaimed'])
        self.assertEqual(parent.pinned(self.b['stagedCandidate']), self.native)

    def test_copy_failure_keeps_both_input_and_failure_evidence(self):
        def fail(*a, **kw): raise OSError('HOST copy failure')
        with self.stopped_lock() as fd, patch.object(ro, 'put', side_effect=fail):
            with self.assertRaises(OSError):
                ro.preserve_stopped(self.b, self.window / 'preserved-readonly-probe-input', self.api, mode='normal', lock_fd=fd)
        self.assertTrue((self.window / 'preserved-readonly-probe-input/failed.json').is_file())
        self.assertEqual(parent.pinned(self.b['stagedCandidate']), self.native)

    def test_stopped_owner_refusal_precedes_copy(self):
        with self.stopped_lock() as fd:
            self.stopped = False
            with self.assertRaises(RuntimeError):
                ro.preserve_stopped(self.b, self.window / 'preserved-readonly-probe-input', self.api, mode='normal', lock_fd=fd)
        self.assertFalse((self.window / 'preserved-readonly-probe-input').exists())

    def test_preserved_copy_drift_never_becomes_release_fact(self):
        with self.stopped_lock() as fd:
            ref = ro.preserve_stopped(self.b, self.window / 'preserved-readonly-probe-input', self.api, mode='normal', lock_fd=fd)
            (self.window / 'preserved-readonly-probe-input/PTEditorPointerV2').write_bytes(b'changed')
            with self.assertRaises(RuntimeError): ro.release_facts(self.b, ref, self.api, mode='normal', lock_fd=fd)
        self.assertEqual(parent.pinned(self.b['stagedCandidate']), self.native)

    def test_retained_input_cannot_impersonate_the_preserved_copy(self):
        with self.stopped_lock() as fd:
            ref = ro.preserve_stopped(self.b, self.window / 'preserved-readonly-probe-input', self.api, mode='normal', lock_fd=fd)
            value = json.loads(parent.pinned(ref)); value['preservedProbe'] = self.b['stagedCandidate']
            Path(ref['path']).write_text(json.dumps(value)); changed = parent.read(ref['path'])[1]
            with self.assertRaises(RuntimeError): ro.release_facts(self.b, changed, self.api, mode='normal', lock_fd=fd)


class ReadonlyFreezeTests(unittest.TestCase):
    prepare = ReadonlyProbeTests.prepare

    def setUp(self):
        ReadonlyProbeTests.setUp(self)
        import editor_pointer_binding as binder
        import uuid
        self.binder = binder
        (self.root / 'SOURCE-FREEZE.json').rename(self.root / 'HOST-PREVIOUS-FIXTURE-FREEZE.json')
        helper = Path(__file__).resolve().parents[3] / 'outputs/ProTracker-2.4G-editor-firstmouse-postmove-refusal-20261022-r3/frozen-source/canonical_profile_session_files.py'
        if not helper.is_file(): self.skipTest('Qualified helper source unavailable')
        own = self.root / 'canonical_profile_session_files.py'; own.write_bytes(helper.read_bytes())
        self.assertEqual(parent.digest(own.read_bytes()), binder.SAVED_FILE_HELPER)
        sources = dict(self.freeze['sources']); sources[str(own)] = parent.read(own)[1]
        for name in binder.LIFECYCLE:
            path = parent.TOOLS / (name + '.py'); sources[str(path)] = parent.read(path)[1]
        for name in ('SOURCE-PROVENANCE.json', 'publish_mouse_observation.py', 'clock_runtime.py'):
            path = self.root / name
            path.write_bytes(b'{"HOST_ONLY":true}\n' if path.suffix == '.json' else b'# HOST source; never imported\n')
            sources[str(path)] = parent.read(path)[1]
        ready_ref = volume.save(self.root / 'HOST-SYNTHETIC-READINESS.json', {'HOST_ONLY_NOT_APPROVAL': True})
        self.plan = dict(self.freeze, state=binder.READONLY_PLAN, acceptedByRoot=True,
            sources=sources, serviceReadinessRef=ready_ref,
            newWindow='/private/tmp/pt24g-readonly-future-window-host-' + uuid.uuid4().hex)
        self.events = []; self.readiness_error = False
        @contextmanager
        def locks():
            self.events.append('both-original-locks-HOST-stand-in'); yield
        def ready(args):
            self.events.append('original-approved-readiness-HOST-stand-in')
            if self.readiness_error: raise RuntimeError('HOST current readiness refusal')
        self.extras = {'canonical_profile_session_files': SimpleNamespace(locks=locks),
                       'service_readiness': SimpleNamespace(require_human_approved_service=ready)}

    def test_distinct_freeze_under_both_locks_binds_exact_readonly_preparation(self):
        ref = self.binder.freeze_readonly_sources(self.plan, self.extras)
        record = json.loads(parent.pinned(ref))
        self.assertEqual(record['state'], ro.FROZEN_STATE)
        self.assertEqual(record['readonlyPreparationRef'], self.b['readonlyPreparationRef'])
        self.assertEqual(len(record['modulePaths']), 23)
        self.assertFalse(record['runtimeAdmission']); self.assertEqual(record['targetOperations'], 0)
        self.assertEqual(self.events, ['both-original-locks-HOST-stand-in', 'original-approved-readiness-HOST-stand-in'])
        self.assertFalse(Path(self.plan['newWindow']).exists())
        with self.assertRaises(RuntimeError): self.binder.freeze_readonly_sources(self.plan, self.extras)
        self.assertEqual(len(self.events), 2)

    def test_readiness_refusal_leaves_no_freeze_or_window(self):
        self.readiness_error = True
        with self.assertRaises(RuntimeError): self.binder.freeze_readonly_sources(self.plan, self.extras)
        self.assertFalse((self.root / 'SOURCE-FREEZE.json').exists())
        self.assertFalse(Path(self.plan['newWindow']).exists())

    def test_missing_lifecycle_source_refuses_before_readiness(self):
        del self.plan['sources'][str(parent.TOOLS / 'editor_capture_settlement.py')]
        with self.assertRaises(RuntimeError): self.binder.freeze_readonly_sources(self.plan, self.extras)
        self.assertFalse(self.events)

    def test_different_original_helper_body_is_not_imported_or_used(self):
        path = self.root / 'canonical_profile_session_files.py'; path.write_bytes(b'# altered helper\n')
        self.plan['sources'][str(path)] = parent.read(path)[1]
        with self.assertRaises(RuntimeError): self.binder.freeze_readonly_sources(self.plan, self.extras)
        self.assertFalse(self.events)

    def test_existing_plan_or_preparation_cannot_substitute_for_new_source_review(self):
        for state in (self.binder.PLAN, volume.STATE):
            plan = dict(self.plan, state=state)
            with self.assertRaises(RuntimeError): self.binder.freeze_readonly_sources(plan, self.extras)
        self.assertFalse(self.events)

    def test_changed_readonly_input_refuses_before_readiness(self):
        Path(self.b['stagedCandidate']['path']).chmod(0o700)
        with self.assertRaises(RuntimeError): self.binder.freeze_readonly_sources(self.plan, self.extras)
        self.assertFalse(self.events)


if __name__ == '__main__': unittest.main()
