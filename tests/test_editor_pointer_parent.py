"""Synthetic admission and parent boundaries; no installed harness is imported."""
import asyncio
import copy
from contextlib import contextmanager
import fcntl
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import editor_pointer_parent as parent


class ParentBindingTests(unittest.TestCase):
    def setUp(self):
        self.bodies = {}
        self.native = b'synthetic-v2'; self.source = b'synthetic-c'; self.editor = b'synthetic-editor'
        for name, body in [('NATIVE', self.native), ('SOURCE', self.source), ('EDITOR', self.editor)]:
            p = patch.object(parent, name, parent.digest(body)); p.start(); self.addCleanup(p.stop)
        self.modules = {}
        private = Path('/private/tmp/pt24g-parent-fixture-source')
        local = {'capture_runner', 'capture_case_mounted', 'first_mouse_api',
                 'capture_support', 'root_observation'}
        shared = {'launcher_guard', 'launcher_access', 'bridge_checks', 'shared_guest', 'emulator'}
        for name in parent.MODULES:
            if name in local: path = private / (name + '.py')
            elif name in shared: path = parent.HARNESS / 'scripts' / (name + '.py')
            elif name.startswith('amiberry_mcp.'):
                path = parent.HARNESS / '.venv/lib/python3.14/site-packages/amiberry_mcp' / (name.split('.')[-1] + '.py')
            else: path = parent.TOOLS / (name + '.py')
            self.modules[name] = str(path)
        sources = {path: self.ref(path, b'synthetic module') for path in self.modules.values()}
        sources[str(Path(parent.__file__))] = self.ref(parent.__file__, b'synthetic parent')
        candidate = self.ref('/fixture/editor', self.editor)
        mounted = self.ref('/fixture/volume/PT24GEdit', self.editor)
        self.freeze = {'state': parent.FROZEN_STATE, 'sources': sources,
                       'modulePaths': self.modules, 'newWindow': '/private/tmp/pt24g-parent-fixture-window'}
        proof = {'state': 'ARCHIVED_ACTUAL_BUILD_GEOMETRY_SOURCE_REVIEW_ONLY',
                 'nativeCellCenter': [218, 424], 'expectedRow': 4, 'expectedChannel': 1, 'expectedField': 0,
                 'clickDispatchUsesUnchangedIntuiMessageCoordinates': True, 'candidate': candidate,
                 'nativeSourceCopies': {'fixture': {'newCopy': self.ref('/fixture/editor.c', b'geometry')}}}
        receipt = {'state': 'PASSIVE_EDITOR_V2_SOURCE_AND_NATIVE_COMPILE_HOST_CHECKED_NOT_EXECUTED',
                   'binary': {'bytes': len(self.native), 'sha256': parent.digest(self.native)[1]},
                   'sourcePinsEqualBeforeAfter': {'tools/diagnostics/native_editor_pointer_snapshot.c':
                       {'bytes': len(self.source), 'sha256': parent.digest(self.source)[1]}}}
        blank = {'state': 'ROOT_ADMITTED_LOCAL_VOLUME_EDITOR_CAPTURE_ONLY', 'ownerRunID': 'fixture-owner',
                 'sourcePins': sources, 'mountedCandidate': mounted, 'candidate': candidate,
                 'outputDirectory': self.freeze['newWindow'] + '/blank-capture-parent-fixture-r1',
                 'modulePaths': self.modules, 'activeProfile': self.ref('/fixture/profile', b'profile'),
                 'allowedActions': {'fixture': 1}, 'limits': {'fixture': 2}}
        probe = {'ownerRunID': 'fixture-owner', 'sourcePins': sources, 'editorCandidate': mounted,
                 'windowDirectory': self.freeze['newWindow'],
                 'nativeSource': self.ref('/fixture/probe.c', self.source),
                 'nativeCandidate': self.ref('/fixture/probe', self.native),
                 'stagedCandidate': self.ref(parent.HARNESS / 'runtime/Dev/Tests/pt24g-editor-pointer-fixture-owner/PTEditorPointerV2', self.native),
                 'compileReceipt': self.jsonref('/fixture/compile', receipt),
                 'guestCandidate': 'Dev:Tests/pt24g-editor-pointer-fixture-owner/PTEditorPointerV2'}
        probe['namespaceStageRef'] = self.jsonref('/fixture/stage', {'stagedCandidate': probe['stagedCandidate']})
        p = patch('editor_probe_namespace.validate_stage', side_effect=lambda b, ref: json.loads(self.pin(ref)))
        p.start(); self.addCleanup(p.stop)
        self.b = {'state': 'ROOT_ADMITTED_EDITOR_MOUSE_WITH_PASSIVE_V2', 'acceptedByRoot': True,
                  'target': 'amiberry-030', 'allowedActions': copy.deepcopy(parent.ALLOWED),
                  'limits': copy.deepcopy(parent.LIMITS), 'nativeSize': [640, 512],
                  'ownerRunID': 'fixture-owner', 'sourcePins': sources,
                  'sourceFreezeRef': self.jsonref('/fixture/freeze', self.freeze),
                  'nativeGeometryProofRef': self.jsonref('/fixture/proof', proof),
                  'blankBinding': blank, 'passiveProbe': probe, 'mountedCandidate': mounted,
                  'outputDirectory': blank['outputDirectory'], 'runLabel': 'blank-capture-parent-fixture-r1'}

    def ref(self, path, body):
        self.bodies[str(path)] = body
        return {'path': str(path), 'bytes': len(body), 'sha256': hashlib.sha256(body).hexdigest(),
                'fullFacts': {'synthetic': True}}

    def jsonref(self, path, value): return self.ref(path, json.dumps(value).encode())

    def pin(self, ref):
        body = self.bodies[ref['path']]
        if parent.digest(body) != (ref['bytes'], ref['sha256']): raise RuntimeError('synthetic pin drift')
        return body

    def validate(self): return parent.validate_binding(self.b, pin=self.pin)

    def test_new_exact_associations_validate_without_importing_harness(self):
        with patch.object(parent.importlib, 'import_module', side_effect=AssertionError('no import')):
            self.assertEqual(self.validate(), self.freeze)

    def test_old_scope_or_changed_action_budget_refuses(self):
        for key, value in [('state', 'ROOT_ADMITTED_CURRENT_EDITOR_FIRST_MOUSE_ONLY'),
                           ('target', 'real-a1200'), ('acceptedByRoot', False),
                           ('limits', dict(parent.LIMITS, rootObservation=61))]:
            before = self.b[key]; self.b[key] = value
            with self.subTest(key=key), self.assertRaises(RuntimeError): self.validate()
            self.b[key] = before

    def test_stale_freeze_state_refuses(self):
        v = dict(self.freeze, state='ROOT_FROZEN_EDITOR_FIRST_MOUSE_SESSION_SOURCE_NOT_EXECUTED')
        self.b['sourceFreezeRef'] = self.jsonref('/fixture/stale-freeze', v)
        with self.assertRaises(RuntimeError): self.validate()

    def test_parent_omitted_from_freeze_refuses(self):
        del self.b['sourcePins'][str(Path(parent.__file__))]
        self.b['sourceFreezeRef'] = self.jsonref('/fixture/newfreeze', self.freeze)
        with self.assertRaises(RuntimeError): self.validate()

    def test_unpinned_or_wrong_module_resolution_refuses_before_import(self):
        self.modules['editor_pointer_probe'] = '/fixture/other-probe.py'
        self.b['sourceFreezeRef'] = self.jsonref('/fixture/newfreeze', self.freeze)
        with self.assertRaises(RuntimeError): self.validate()

    def test_changed_native_geometry_proof_refuses(self):
        proof = json.loads(self.pin(self.b['nativeGeometryProofRef']))
        proof['nativeCellCenter'] = [219, 425]
        self.b['nativeGeometryProofRef'] = self.jsonref('/fixture/wrong-proof', proof)
        with self.assertRaises(RuntimeError): self.validate()

    def test_blank_or_probe_owner_change_refuses(self):
        for name in ('blankBinding', 'passiveProbe'):
            self.b[name]['ownerRunID'] = 'other'
            with self.subTest(name=name), self.assertRaises(RuntimeError): self.validate()
            self.b[name]['ownerRunID'] = 'fixture-owner'

    def test_wrong_editor_native_source_or_staged_bytes_refuse(self):
        for outer, role in [(self.b['blankBinding'], 'candidate'),
                            (self.b['passiveProbe'], 'nativeSource'),
                            (self.b['passiveProbe'], 'nativeCandidate'),
                            (self.b['passiveProbe'], 'stagedCandidate')]:
            old = outer[role]; outer[role] = self.ref(old['path'], b'wrong')
            with self.subTest(role=role), self.assertRaises(RuntimeError): self.validate()
            outer[role] = old; self.bodies[old['path']] = {'candidate': self.editor,
                'nativeSource': self.source, 'nativeCandidate': self.native, 'stagedCandidate': self.native}[role]

    def test_wrong_probe_namespace_or_compile_receipt_refuses(self):
        p = self.b['passiveProbe']; p['guestCandidate'] = 'Dev:unrelated'
        with self.assertRaises(RuntimeError): self.validate()
        p['guestCandidate'] = 'Dev:Tests/pt24g-editor-pointer-fixture-owner/PTEditorPointerV2'
        receipt = json.loads(self.pin(p['compileReceipt'])); receipt['binary']['bytes'] += 1
        p['compileReceipt'] = self.jsonref('/fixture/wrong-compile', receipt)
        with self.assertRaises(RuntimeError): self.validate()

    def test_changed_output_binding_refuses_before_original_operation(self):
        self.b['outputDirectory'] += '-other'
        with self.assertRaises(RuntimeError): self.validate()

    def test_source_body_drift_refuses(self):
        self.bodies[self.modules['launcher_access']] = b'changed'
        with self.assertRaises(RuntimeError): self.validate()

    def test_loader_refuses_shadowed_module_before_import(self):
        with patch.object(parent, 'pinned', side_effect=self.pin), \
             patch.object(parent.importlib.util, 'find_spec', return_value=SimpleNamespace(origin='/fixture/shadow.py')), \
             patch.object(parent.importlib, 'import_module') as imported:
            with self.assertRaises(RuntimeError): parent.load_original_modules(self.b, self.freeze)
            imported.assert_not_called()

    def test_loader_rechecks_actual_module_path_and_source_after_import(self):
        for mode in ('wrong-file', 'pin-drift'):
            name = next(iter(self.modules)); path = self.modules[name]
            def imported(_):
                if mode == 'pin-drift': self.bodies[path] = b'changed while importing'
                return SimpleNamespace(__file__=path if mode == 'pin-drift' else '/fixture/other.py')
            with self.subTest(mode=mode), patch.object(parent, 'pinned', side_effect=self.pin), \
                 patch.object(parent.importlib.util, 'find_spec', return_value=SimpleNamespace(origin=path)), \
                 patch.object(parent.importlib, 'import_module', side_effect=imported):
                with self.assertRaises(RuntimeError): parent.load_original_modules(self.b, self.freeze)
            self.bodies[path] = b'synthetic module'

    def test_invalid_run_label_is_refused_by_unchanged_original_protocol(self):
        self.b['runLabel'] = 'other-label'
        self.b['outputDirectory'] = self.freeze['newWindow'] + '/other-label'
        self.b['blankBinding']['outputDirectory'] = self.b['outputDirectory']
        with self.assertRaises(ValueError): self.validate()

    def test_stage_receipt_drift_or_other_endpoint_refuses_before_import(self):
        ref = self.b['passiveProbe']['namespaceStageRef']
        self.b['passiveProbe']['namespaceStageRef'] = self.jsonref(ref['path'], {'stagedCandidate': {'path': '/other'}})
        with self.assertRaises(RuntimeError): self.validate()

    def plan(self):
        b=copy.deepcopy(self.b)
        b.update(state=parent.STAGE_PLAN_STATE,allowedActions=copy.deepcopy(parent.STAGE_PLAN_ALLOWED),
                 limits=copy.deepcopy(parent.STAGE_PLAN_LIMITS))
        del b['passiveProbe']['stagedCandidate'];del b['passiveProbe']['namespaceStageRef']
        return b

    def test_stage_plan_validates_only_its_distinct_explicit_scope(self):
        b=self.plan()
        parent.validate_binding(b,pin=self.pin,stage_plan=True)
        with self.assertRaises(RuntimeError):parent.validate_binding(b,pin=self.pin)

    def test_stage_plan_cannot_adopt_an_existing_or_guessed_endpoint(self):
        b=self.plan();b['passiveProbe']['stagedCandidate']=self.b['passiveProbe']['stagedCandidate']
        with self.assertRaises(RuntimeError):parent.validate_binding(b,pin=self.pin,stage_plan=True)

    def test_stage_plan_without_extra_host_write_scope_refuses(self):
        b=self.plan();b['allowedActions']=copy.deepcopy(parent.ALLOWED)
        with self.assertRaises(RuntimeError):parent.validate_binding(b,pin=self.pin,stage_plan=True)


class ParentExecutionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name); (root/'runtime').mkdir(); (root/'runtime/test.lock').touch()
        self.lock = root/'runtime/test.lock'
        p = patch.object(parent, 'HARNESS', root); p.start(); self.addCleanup(p.stop)
        self.events = []
        self.owner = 'fixture-owner'
        self.status = {'phase': 'READY', 'holds': [], 'inflight': None}
        self.failure = None
        outer = self

        class Guard:
            def __init__(self, root): outer.events.append('guard')
            def status(self): return outer.status
            @contextmanager
            def _locked(self): yield None, {}
            def _owner(self, state, lease, target): outer.events.append('owner-verified')

        @contextmanager
        def operation(*args, **kwargs):
            outer.events.append('operation-enter')
            try: yield outer.frame
            except BaseException:
                outer.events.append('operation-error'); raise
            else: outer.events.append('operation-return')

        class Original:
            def __init__(self, binding, frame):
                self.binding, self.frame = binding, frame
                outer.events.append('original-api')
            def owner(self, runid):
                self_owner = outer.owner
                if runid != self_owner: raise RuntimeError('synthetic owner loss')
                outer.events.append('live-owner')

        async def blank(binding, out, api):
            outer.events.append('blank')
            if outer.failure == 'blank': raise RuntimeError('synthetic blank error')
            if outer.failure == 'owner-after-blank': outer.owner = 'other'
            return {'synthetic-blank': True}

        def mouse(original, binding, frame, **kwargs):
            outer.events.append('mouse-api')
            self.assertIs(frame, original.frame)
            return SimpleNamespace(original=original, binding=binding, frame=frame)

        async def composed(binding, blank, out, mouse, original):
            outer.events.append('composed')
            self.assertIs(mouse.frame, original.frame)
            if outer.failure == 'composed': raise RuntimeError('synthetic probe error')
            if outer.failure == 'cancel': raise asyncio.CancelledError()
            return {'state': 'SYNTHETIC_ONLY', 'attempts': {}}

        self.frame = {'synthetic': True}
        self.b = {'ownerRunID': 'fixture-owner', 'blankBinding': {}, 'outputDirectory': '/fixture/output'}
        self.d = SimpleNamespace(guard=Guard, lease_input=lambda *a: {'run_id': self.owner},
            install_client=lambda: self.events.append('client-hook'),
            install_ipc=lambda *a: self.events.append('ipc-hook'), operation=operation,
            original_api=Original, blank_case=blank, mouse_api=mouse,
            move_request=object(), click_request=object(), composed_case=composed)
        p = patch.object(parent, 'validate_binding', side_effect=lambda b,**kw: self.events.append('validated'))
        p.start(); self.addCleanup(p.stop)

    def test_one_original_operation_composes_blank_then_mouse_in_same_frame(self):
        result = parent.execute(self.b, self.d)
        self.assertEqual(result['state'], 'SYNTHETIC_ONLY')
        self.assertLess(self.events.index('validated'), self.events.index('client-hook'))
        self.assertLess(self.events.index('blank'), self.events.index('composed'))
        self.assertEqual(self.events.count('operation-enter'), 1)
        self.assertEqual(self.events[-1], 'operation-return')

    def test_hold_inflight_or_changed_owner_never_installs_or_dispatches(self):
        for status, owner in [({'phase':'HOLD','holds':[{}],'inflight':None}, 'fixture-owner'),
                              ({'phase':'READY','holds':[],'inflight':{}}, 'fixture-owner'),
                              (self.status, 'other')]:
            self.events.clear(); self.status = status; self.owner = owner
            with self.assertRaises(RuntimeError): parent.execute(self.b, self.d)
            self.assertNotIn('client-hook', self.events); self.assertNotIn('operation-enter', self.events)

    def test_failed_blank_never_starts_probe_or_retries(self):
        self.failure = 'blank'
        with self.assertRaises(RuntimeError): parent.execute(self.b, self.d)
        self.assertEqual(self.events.count('blank'), 1)
        self.assertNotIn('composed', self.events)
        self.assertEqual(self.events[-1], 'operation-error')

    def test_failed_probe_propagates_through_original_operation_without_cleanup(self):
        self.failure = 'composed'
        with self.assertRaises(RuntimeError): parent.execute(self.b, self.d)
        self.assertEqual(self.events.count('composed'), 1)
        self.assertEqual(self.events[-1], 'operation-error')

    def test_changed_owner_after_blank_prevents_mouse_phase(self):
        self.failure = 'owner-after-blank'
        with self.assertRaises(RuntimeError): parent.execute(self.b, self.d)
        self.assertNotIn('mouse-api', self.events); self.assertNotIn('composed', self.events)

    def test_cancellation_propagates_without_retry_and_releases_only_command_lock(self):
        self.failure = 'cancel'
        with self.assertRaises(asyncio.CancelledError): parent.execute(self.b, self.d)
        self.assertEqual(self.events.count('composed'), 1)
        self.assertEqual(self.events[-1], 'operation-error')
        with parent.command_lock(self.lock): pass

    def test_validation_failure_under_lock_prevents_guard_and_releases_lock(self):
        with patch.object(parent, 'validate_binding', side_effect=RuntimeError('synthetic changed source')):
            with self.assertRaises(RuntimeError): parent.execute(self.b, self.d)
        self.assertEqual(self.events, [])
        with parent.command_lock(self.lock): pass

    def test_busy_command_lock_refuses_before_validation_or_any_dependency(self):
        with self.lock.open('r+') as stream:
            fcntl.flock(stream.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
            with self.assertRaises(BlockingIOError): parent.execute(self.b, self.d)
        self.assertEqual(self.events, [])

    def test_unsafe_command_lock_refuses_without_creating_replacement(self):
        self.lock.write_bytes(b'unexpected')
        with self.assertRaises(RuntimeError): parent.execute(self.b, self.d)
        self.assertEqual(self.lock.read_bytes(), b'unexpected')
        self.assertEqual(self.events, [])

    def staged_dependencies(self, *, fail=False):
        window=Path(self.temp.name)/'window';window.mkdir()
        self.b.update(state=parent.STAGE_PLAN_STATE, outputDirectory=str(window/'blank-capture-fixture-r1'),
                      passiveProbe={'ownerRunID':'fixture-owner','windowDirectory':str(window)})
        def stage(binding,out,api,*,lock_fd):
            self.events.append('stage')
            self.assertEqual(os.fstat(lock_fd).st_ino,self.lock.lstat().st_ino)
            api.stage_owner('fixture-owner')
            if fail:raise RuntimeError('synthetic failed stage')
            return {'path':'FAKE_HOST_STAGE_ONLY','sha256':'FAKE'}
        self.d.namespace_stage=stage
        self.d.namespace_validate=lambda b,ref: {'stagedCandidate':{'path':'FAKE_HOST_NATIVE_ONLY'}}
        return window

    def test_actual_stage_result_is_bound_before_blank_in_original_frame(self):
        window=self.staged_dependencies()
        parent.execute(self.b,self.d)
        self.assertLess(self.events.index('stage'),self.events.index('blank'))
        self.assertEqual(self.events.count('operation-enter'),1)
        child=json.loads((window/'mouse-parent-staged-binding.json').read_text())
        self.assertEqual(child['passiveProbe']['namespaceStageRef']['path'],'FAKE_HOST_STAGE_ONLY')
        self.assertEqual(child['state'],'ROOT_ADMITTED_EDITOR_MOUSE_WITH_PASSIVE_V2')
        self.assertNotIn('namespaceStageRef',self.b['passiveProbe'])

    def test_stage_failure_propagates_without_blank_or_retry(self):
        self.staged_dependencies(fail=True)
        with self.assertRaises(RuntimeError):parent.execute(self.b,self.d)
        self.assertEqual(self.events.count('stage'),1);self.assertNotIn('blank',self.events)
        self.assertEqual(self.events[-1],'operation-error')

    def test_stage_deadline_refuses_before_stage_owner_dispatch(self):
        self.staged_dependencies()
        with patch.object(parent.time,'monotonic',side_effect=[100.,200.]):
            with self.assertRaises(RuntimeError):parent.execute(self.b,self.d)
        self.assertNotIn('blank',self.events);self.assertEqual(self.events.count('stage'),1)


class BootstrapFileTests(unittest.TestCase):
    @unittest.skipUnless(Path('/usr/bin/python3').is_file(), 'No alternate system Python')
    def test_actual_other_interpreter_refuses_before_app_annotation_imports(self):
        child = subprocess.run(['/usr/bin/python3', '-B', parent.__file__, '--help'],
                               capture_output=True, text=True, timeout=20)
        self.assertNotEqual(child.returncode, 0)
        self.assertEqual(child.stdout, '')
        self.assertIn('Original harness Python3.14 clock domain required before target imports', child.stderr)
        self.assertNotIn('TypeError', child.stderr)
        self.assertNotIn('ModuleNotFoundError', child.stderr)

    def test_actual_file_pin_then_body_drift_and_symlink_refusals(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'input'; path.write_bytes(b'original')
            body, ref = parent.read(path)
            self.assertEqual(parent.pinned(ref), body)
            path.write_bytes(b'changed')
            with self.assertRaises(RuntimeError): parent.pinned(ref)
            link = Path(directory)/'alias'; link.symlink_to(path)
            with self.assertRaises(RuntimeError): parent.read(link)

    def test_oversized_file_and_extra_reference_fields_refuse(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'input'; path.write_bytes(b'012345')
            with self.assertRaises(RuntimeError): parent.read(path, 5)
            _, ref = parent.read(path); ref['extra'] = True
            with self.assertRaises(RuntimeError): parent.pinned(ref)

    def test_other_runtime_refuses_before_any_original_import(self):
        with patch.object(parent.sys, 'executable', '/fixture/other-python'), \
             patch.object(parent.importlib, 'import_module') as imported:
            with self.assertRaises(RuntimeError): parent.require_harness_runtime()
            imported.assert_not_called()


if __name__ == '__main__': unittest.main()
