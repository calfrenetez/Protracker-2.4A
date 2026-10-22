"""Private supervisor fixtures; original guard and child execution are synthetic."""
from contextlib import contextmanager
import copy
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
import editor_pointer_dispatch as dispatch
from editor_probe_namespace import save


class DispatchTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='pt24g-dispatch-HOST-', dir='/private/tmp')
        self.addCleanup(self.temp.cleanup); self.root = Path(self.temp.name)
        self.h = self.root / 'harness'; (self.h / 'runtime').mkdir(parents=True)
        (self.h / 'runtime/test.lock').touch()
        self.window = self.root / 'window'; self.window.mkdir()
        for module in (parent, dispatch):
            p = patch.object(module, 'HARNESS', self.h); p.start(); self.addCleanup(p.stop)
        self.events = []; self.phase = 'READY'; self.lease_owner = 'FAKE_HOST_OWNER'
        self.child_rc = 0; self.child_error = None; self.spawn_error = None
        self.validator_error = None; self.sources = {str(Path(dispatch.__file__)): parent.read(dispatch.__file__)[1]}
        path = self.h / 'scripts/launcher_guard.py'; path.parent.mkdir()
        path.write_bytes(b'FAKE_HOST_SOURCE_NEVER_IMPORTED'); self.sources[str(path)] = parent.read(path)[1]
        self.b = {'state': parent.STAGE_PLAN_STATE, 'ownerRunID': self.lease_owner,
                  'outputDirectory': str(self.window / 'blank-capture-dispatch-fixture-r1'),
                  'sourcePins': self.sources, 'limits': copy.deepcopy(parent.STAGE_PLAN_LIMITS),
                  'blankBinding': {'limits': {'whole': 300},
                                   'modulePaths': {'launcher_guard': str(path)}}}
        self.ref = save(self.window / 'mouse-parent-binding.json', self.b)
        save(self.window / 'private-lease.json', {'lease': {'run_id': self.lease_owner, 'token': 'FAKE_HOST_ONLY'}})
        outer = self
        def validate(binding, *, stage_plan):
            outer.events.append('validate')
            self.assertTrue(stage_plan); self.assertEqual(binding['state'], parent.STAGE_PLAN_STATE)
            if self.validator_error: raise self.validator_error
            return {'state': parent.FROZEN_STATE}
        p = patch.object(parent, 'validate_binding', side_effect=validate)
        p.start(); self.addCleanup(p.stop)
        # Tests must not accidentally inherit live target authority.
        p = patch.dict(os.environ, {}, clear=True); p.start(); self.addCleanup(p.stop)
        class Guard:
            def __init__(self, root): outer.events.append('guard')
            def status(self):
                return {'phase': outer.phase, 'holds': [{}] if outer.phase == 'HOLD' else [], 'inflight': None}
            @contextmanager
            def _locked(self): yield None, {}
            def _owner(self, state, lease, target):
                outer.events.append('owner'); outer.assertEqual(target, 'amiberry-030')
                if lease['run_id'] != outer.lease_owner: raise RuntimeError('FAKE original owner refusal')
        class Child:
            pid = 999999
            @property
            def returncode(self): return outer.child_rc
            def wait(self, *, timeout):
                outer.events.append(('wait', timeout))
                if outer.child_error: raise outer.child_error
                return self.returncode
            def kill(self): raise AssertionError('No hidden child kill')
            def terminate(self): raise AssertionError('No hidden child terminate')
            def __enter__(self): raise AssertionError('No child context-manager lifetime change')
            def __exit__(self, *args): raise AssertionError('No child context-manager lifetime change')
        def popen(argv, **kwargs):
            outer.events.append('spawn')
            # The actual supplied lock must be available to the child.
            with parent.command_lock(self.h / 'runtime/test.lock'): pass
            self.argv = argv; self.kwargs = kwargs
            self.assertEqual(kwargs['env']['AMIGA_LAUNCH_LEASE'], str(self.window / 'private-lease.json'))
            self.assertNotIn('AMIGA_LAUNCH_INFLIGHT', kwargs['env'])
            self.assertTrue((self.window / 'capture-dispatch-intent.json').is_file())
            if self.spawn_error: raise self.spawn_error
            return Child()
        self.d = SimpleNamespace(guard=Guard, decode=json.loads, popen=popen, clock=lambda: 100.)

    def run_dispatch(self): return dispatch.execute(self.ref, self.d)

    def test_one_child_uses_actual_parent_ref_and_lease_with_lock_released(self):
        result = self.run_dispatch()
        self.assertEqual(self.events, ['validate', 'guard', 'owner', 'spawn', ('wait', 720)])
        self.assertEqual(self.argv[:3], [str(self.h / '.venv/bin/python'), '-B', str(Path(parent.__file__))])
        self.assertEqual(json.loads(self.argv[-1]), self.ref)
        self.assertEqual(self.kwargs['cwd'], self.h)
        self.assertEqual(result['returncode'], 0); self.assertFalse(result['guardReleaseClaimed'])
        self.assertNotIn('AMIGA_LAUNCH_LEASE', os.environ)

    def test_nonzero_child_is_retained_and_recorded_without_retry(self):
        self.child_rc = 7; result = self.run_dispatch()
        self.assertEqual(result['returncode'], 7); self.assertFalse(result['retryAllowed'])
        self.assertEqual(self.events.count('spawn'), 1)
        self.assertTrue((self.window / 'capture.stderr').exists())

    def test_timeout_keeps_child_and_intent_without_signal_or_replay(self):
        self.child_error = subprocess.TimeoutExpired('FAKE_HOST_CHILD', 720)
        with self.assertRaises(subprocess.TimeoutExpired): self.run_dispatch()
        saved = json.loads((self.window / 'capture-supervisor-timeout.json').read_text())
        self.assertEqual(saved['pid'], 999999); self.assertEqual(saved['signalsSent'], 0)
        failure = json.loads((self.window / 'capture-supervisor-failure.json').read_text())
        self.assertEqual(failure['errorType'], 'TimeoutExpired')
        with self.assertRaises(RuntimeError): self.run_dispatch()
        self.assertEqual(self.events.count('spawn'), 1)

    def test_interruption_keeps_child_without_a_hidden_signal(self):
        self.child_error = KeyboardInterrupt()
        with self.assertRaises(KeyboardInterrupt): self.run_dispatch()
        failure = json.loads((self.window / 'capture-supervisor-failure.json').read_text())
        self.assertEqual(failure['pid'], 999999); self.assertEqual(failure['signalsSent'], 0)

    def test_spawn_failure_consumes_intent_and_is_not_replayed(self):
        self.spawn_error = OSError('FAKE spawn failure')
        with self.assertRaises(OSError): self.run_dispatch()
        failure = json.loads((self.window / 'capture-supervisor-failure.json').read_text())
        self.assertIsNone(failure['pid'])
        with self.assertRaises(RuntimeError): self.run_dispatch()
        self.assertEqual(self.events.count('spawn'), 1)

    def test_parent_validation_failure_precedes_original_guard_or_child(self):
        self.validator_error = RuntimeError('FAKE source/parent refusal')
        with self.assertRaises(RuntimeError): self.run_dispatch()
        self.assertEqual(self.events, ['validate'])

    def test_wrong_lease_or_held_guard_never_spawns(self):
        self.lease_owner = 'OTHER'
        with self.assertRaises(RuntimeError): self.run_dispatch()
        self.lease_owner = self.b['ownerRunID']; self.phase = 'HOLD'
        with self.assertRaises(RuntimeError): self.run_dispatch()
        self.assertNotIn('spawn', self.events)
        self.assertFalse((self.window / 'capture-dispatch-intent.json').exists())

    def test_guard_source_body_drift_refuses_before_intent(self):
        Path(self.b['blankBinding']['modulePaths']['launcher_guard']).write_bytes(b'drift')
        with self.assertRaises(RuntimeError): self.run_dispatch()
        self.assertNotIn('spawn', self.events)

    def test_existing_or_dangling_output_never_borrows_dispatch(self):
        (self.window / 'capture.stdout').symlink_to(self.window / 'ABSENT')
        with self.assertRaises(RuntimeError): self.run_dispatch()
        self.assertEqual(self.events, ['validate'])

    def test_symlink_window_refuses_before_guard_or_child(self):
        actual = self.root / 'actual-window'
        self.window.rename(actual); self.window.symlink_to(actual, target_is_directory=True)
        # Repin the same binding body; the directory check must reject the alias.
        self.ref = parent.read(self.window / 'mouse-parent-binding.json')[1]
        with self.assertRaises(RuntimeError): self.run_dispatch()
        self.assertEqual(self.events, ['validate'])

    def test_inherited_lease_or_inflight_refuses_before_original_guard(self):
        for name in dispatch.INHERITED_AUTHORITY:
            with patch.dict(os.environ, {name: 'FAKE_HOST_INHERITED'}):
                with self.assertRaises(RuntimeError): self.run_dispatch()
        self.assertNotIn('guard', self.events)

    def test_unfrozen_dispatcher_or_other_binding_path_refuses(self):
        other = dict(self.b); other['sourcePins'] = {}
        ref = save(self.window / 'OTHER.json', other)
        with self.assertRaises(RuntimeError): dispatch.execute(ref, self.d)
        self.assertNotIn('guard', self.events)

    def test_original_subphase_budget_is_not_silently_extended(self):
        with patch.object(dispatch, 'SUPERVISOR_SECONDS', 600):
            with self.assertRaises(RuntimeError): self.run_dispatch()
        self.assertEqual(parent.STAGE_PLAN_LIMITS['whole'], 240)
        self.assertEqual(parent.STAGE_PLAN_LIMITS['rootObservation'], 60)
        self.assertNotIn('guard', self.events)

    def test_shadowed_original_guard_refuses_before_import(self):
        with patch.object(dispatch.importlib.util, 'find_spec', return_value=SimpleNamespace(origin='/wrong')), \
             patch.object(dispatch.importlib, 'import_module') as imported:
            with self.assertRaises(RuntimeError): dispatch.load_dependencies(self.b)
            imported.assert_not_called()

    def test_actual_alternate_runtime_refuses_before_dispatch_imports(self):
        child = subprocess.run(['/usr/bin/python3', '-B', dispatch.__file__, '--help'],
                               capture_output=True, text=True, timeout=20)
        self.assertNotEqual(child.returncode, 0); self.assertEqual(child.stdout, '')
        self.assertIn('Original harness clock required before dispatcher imports', child.stderr)
        self.assertNotIn('TypeError', child.stderr)


if __name__ == '__main__': unittest.main()
