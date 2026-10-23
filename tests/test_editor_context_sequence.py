"""Real private binding/dispatch algorithms, fake children/host/guest observations."""
import json
from pathlib import Path
import subprocess
import sys
import time
from types import SimpleNamespace
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import editor_context_sequence as sequence
import editor_context_start as start
import editor_context_dispatch as dispatch
import editor_pointer_parent as original
import editor_context_volume as volume
import test_editor_context_lifecycle as life
import test_editor_context_start as fixtures


class ContextSequenceTests(unittest.TestCase):
    prepare = life.ContextLifecycleTests.prepare
    record = fixtures.ContextStartTests.record
    def setUp(self):
        life.ContextLifecycleTests.setUp(self)
        for module in (sequence, dispatch):
            p = patch.object(module, 'HARNESS', self.h); p.start(); self.addCleanup(p.stop)
        p = patch.dict(sequence.os.environ, {}, clear=True); p.start(); self.addCleanup(p.stop)
        path = str(Path(sequence.__file__))
        self.freeze['sources'][path] = original.read(path)[1]
        record = self.record(); self.ref = volume.save(self.window / 'actual-start-binding.json', record)
        volume.save(self.window / 'private-lease.json', {'lease': {'run_id': 'fixture-owner', 'HOST_ONLY': True}})
        self.profile_file = self.root / 'HOST-active-profile.uae'
        self.profile_file.write_bytes(self.active)
        self.profile_pin = original.read(self.profile_file)[1]
        prior_pin = original.pinned
        def actual_profile_pin(ref):
            if ref == self.active_ref: return prior_pin(self.profile_pin)
            return prior_pin(ref)
        p = patch.object(original, 'pinned', actual_profile_pin); p.start(); self.addCleanup(p.stop)
        self.option = None; self.children = []; self.dispatches = []; self.clock_calls = 0
        outer = self
        class Binder:
            pid = 777777
            returncode = 0
            def wait(self, timeout):
                outer.assertEqual(timeout, 180)
                if outer.option == 'timeout': raise subprocess.TimeoutExpired('HOST binder', timeout)
                if outer.option == 'wait-error': raise RuntimeError('HOST binder wait error')
                if outer.option == 'nonzero': self.returncode = 1; return
                ref = start.execute(record, outer.d)
                if outer.option == 'profile': outer.profile_file.write_bytes(outer.original)
                if outer.option == 'profile-ctime':
                    before = original.read(outer.profile_file)[1]
                    outer.profile_file.chmod(outer.profile_file.stat().st_mode & 0o777)
                    after = original.read(outer.profile_file)[1]
                    outer.assertEqual(before['sha256'], after['sha256'])
                    outer.assertNotEqual(before['fullFacts']['st_ctime_ns'], after['fullFacts']['st_ctime_ns'])
                if outer.option == 'owner-after': outer.condition = 'owner'
                if outer.option == 'bad-ref': ref = dict(ref, path=str(outer.window / 'FOREIGN.json'))
                outer.binder_stream.write((json.dumps({'ref': ref, 'applicationLaunches': 0,
                                                       'lifecyclePending': True}) + '\n').encode())
        def popen(argv, **kw):
            self.children.append((argv, kw)); self.binder_stream = kw['stdout']
            self.assertEqual(argv[:4], [str(self.h / '.venv/bin/python'), '-B', str(Path(start.__file__)), '--actual-binding-ref'])
            self.assertEqual(json.loads(argv[4]), self.ref)
            self.assertEqual(kw['env']['AMIGA_LAUNCH_LEASE'], str(self.window / 'private-lease.json'))
            self.assertNotIn('AMIGA_LAUNCH_INFLIGHT', kw['env'])
            return Binder()
        class Parent:
            pid = 888888
            returncode = 0
            def wait(self, timeout):
                outer.assertEqual(timeout, 480)
                self.returncode = 3 if outer.option == 'parent-nonzero' else 0
        def parent_popen(argv, **kw):
            self.dispatches.append((argv, kw)); self.assertNotIn('AMIGA_LAUNCH_INFLIGHT', kw['env'])
            return Parent()
        supervisor = SimpleNamespace(guard=self.d.guard, decode=json.loads, popen=parent_popen, clock=time.monotonic)
        def clock():
            self.clock_calls += 1
            return 100 + (31 if self.option == 'gap' and self.clock_calls > 1 else 0)
        self.seq = SimpleNamespace(guard=self.d.guard, decode=json.loads, popen=popen,
            clock=clock, dispatch=lambda ref: dispatch.execute(ref, supervisor))

    def test_one_original_binder_cpu_admission_then_original_dispatch_without_pin_refresh(self):
        result = sequence.execute(self.ref, self.seq)
        self.assertEqual(result['returncode'], 0); self.assertTrue(result['lifecyclePending'])
        self.assertEqual((len(self.children), len(self.dispatches)), (1, 1))
        self.assertEqual(self.events.count('guest'), 1)
        b = json.loads((self.window / 'context-parent-binding.json').read_text())
        self.assertEqual(b['passiveProbe']['activeProfile'], self.start_record['activeProfileRef'])
        self.assertEqual(b['allowedActions']['mouseEvents'], 0)
        value = json.loads(original.pinned(result['ref']))
        self.assertEqual(value['profilePinRefreshes'], 0); self.assertFalse(value['guardReleaseClaimed'])
        with self.assertRaises(RuntimeError): sequence.execute(self.ref, self.seq)
        self.assertEqual((len(self.children), len(self.dispatches)), (1, 1))

    def test_failed_binder_is_not_dispatched_or_replayed(self):
        self.option = 'nonzero'
        with self.assertRaises(RuntimeError): sequence.execute(self.ref, self.seq)
        with self.assertRaises(RuntimeError): sequence.execute(self.ref, self.seq)
        self.assertEqual((len(self.children), len(self.dispatches)), (1, 0))
        self.assertEqual(self.release_calls, 0)

    def test_timed_out_binder_is_retained_without_signal(self):
        self.option = 'timeout'
        with self.assertRaises(subprocess.TimeoutExpired): sequence.execute(self.ref, self.seq)
        value = json.loads((self.window / 'binder-supervisor-timeout.json').read_text())
        self.assertEqual(value['signals'], 0); self.assertEqual(self.dispatches, [])
        with self.assertRaises(RuntimeError): sequence.execute(self.ref, self.seq)
        self.assertEqual(len(self.children), 1)

    def test_wait_error_retains_without_parent_dispatch(self):
        self.option = 'wait-error'
        with self.assertRaises(RuntimeError): sequence.execute(self.ref, self.seq)
        self.assertEqual(self.dispatches, [])
        self.assertTrue((self.window / 'bind-dispatch-failed.json').exists())

    def test_changed_profile_after_binder_never_refreshes_or_dispatches(self):
        self.option = 'profile'
        with self.assertRaises(RuntimeError): sequence.execute(self.ref, self.seq)
        self.assertEqual(self.dispatches, []); self.assertEqual(self.events.count('guest'), 1)
        value = json.loads((self.window / 'bind-dispatch-failed.json').read_text())
        self.assertEqual(value['profilePinRefreshes'], 0)

    def test_same_body_with_changed_ctime_is_still_refused_without_refresh(self):
        self.option = 'profile-ctime'
        with self.assertRaises(RuntimeError): sequence.execute(self.ref, self.seq)
        self.assertEqual(self.dispatches, []); self.assertEqual(self.events.count('guest'), 1)
        value = json.loads((self.window / 'bind-dispatch-failed.json').read_text())
        self.assertEqual(value['profilePinRefreshes'], 0)

    def test_owner_lost_after_binder_prevents_parent_dispatch(self):
        self.option = 'owner-after'
        with self.assertRaises(RuntimeError): sequence.execute(self.ref, self.seq)
        self.assertEqual((len(self.children), len(self.dispatches)), (1, 0))

    def test_long_host_gap_refuses_parent_dispatch(self):
        self.option = 'gap'
        with self.assertRaisesRegex(RuntimeError, 'gap exhausted'): sequence.execute(self.ref, self.seq)
        self.assertEqual(self.dispatches, [])

    def test_wrong_binder_output_cannot_adopt_other_binding(self):
        self.option = 'bad-ref'
        with self.assertRaises(RuntimeError): sequence.execute(self.ref, self.seq)
        self.assertEqual(self.dispatches, [])

    def test_parent_failure_is_preserved_as_nonzero_without_cleanup_or_release(self):
        self.option = 'parent-nonzero'; result = sequence.execute(self.ref, self.seq)
        self.assertEqual(result['returncode'], 3); self.assertEqual(self.release_calls, 0)
        self.assertEqual((len(self.children), len(self.dispatches)), (1, 1))

    def test_inherited_lease_or_frame_refuses_before_child_or_intent(self):
        for key in sequence.AUTHORITY:
            with patch.dict(sequence.os.environ, {key: 'HOST_FOREIGN'}):
                with self.assertRaises(RuntimeError): sequence.execute(self.ref, self.seq)
        self.assertEqual(self.children, [])
        self.assertFalse((self.window / 'bind-dispatch-intent.json').exists())

    def test_changed_owner_prevents_binder_child(self):
        self.condition = 'owner'
        with self.assertRaises(RuntimeError): sequence.execute(self.ref, self.seq)
        self.assertEqual(self.children, [])

    def test_sequence_source_must_be_selected_before_freeze(self):
        path = Path(self.start_record['sourceFreezeRef']['path']); value = json.loads(path.read_text())
        del value['sources'][str(Path(sequence.__file__))]; path.write_text(json.dumps(value))
        self.start_record['sourceFreezeRef'] = original.read(path)[1]
        p = Path(self.ref['path']); p.write_text(json.dumps(self.start_record)); self.ref = original.read(p)[1]
        with self.assertRaises(RuntimeError): sequence.execute(self.ref, self.seq)
        self.assertEqual(self.children, [])

    def test_alternate_runtime_refuses_before_project_imports(self):
        result = subprocess.run(['/usr/bin/python3', '-B', str(Path(sequence.__file__))],
                                capture_output=True, text=True, timeout=10)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Original harness clock required before sequence imports', result.stderr)
        self.assertNotIn('editor_pointer_parent.py', result.stderr)
        self.assertEqual(self.children, [])


if __name__ == '__main__': unittest.main()
