"""One fake child under private fixture locks; no subprocess target activity."""
import json
from pathlib import Path
import subprocess
import sys
import time
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import editor_context_dispatch as dispatch
import editor_context_parent as parent
import editor_context_volume as volume
import editor_pointer_parent as original
import test_editor_context_lifecycle as fixtures


class ContextDispatchTests(unittest.TestCase):
    prepare = fixtures.ContextLifecycleTests.prepare
    def setUp(self):
        fixtures.ContextLifecycleTests.setUp(self)
        p = patch.object(dispatch, 'HARNESS', self.h); p.start(); self.addCleanup(p.stop)
        p = patch.dict(dispatch.os.environ, {}, clear=True); p.start(); self.addCleanup(p.stop)
        self.lease = {'run_id': 'fixture-owner', 'HOST_ONLY': True}
        volume.save(self.window / 'private-lease.json', {'lease': self.lease})
        self.option = None; self.children = []
        outer = self
        class Child:
            pid = 888888
            returncode = 0
            def wait(self, timeout):
                outer.assertEqual(timeout, 480)
                if outer.option == 'timeout': raise subprocess.TimeoutExpired('HOST child', timeout)
                if outer.option == 'wait-error': raise RuntimeError('HOST wait failure')
                self.returncode = 3 if outer.option == 'nonzero' else 0
        def popen(argv, **kw):
            self.events.append('spawn'); self.children.append((argv, kw))
            self.assertEqual(argv[:4], [str(self.h / '.venv/bin/python'), '-B', str(Path(parent.__file__)), '--binding-ref'])
            self.assertEqual(json.loads(argv[4]), self.parent_ref)
            self.assertEqual(kw['env']['AMIGA_LAUNCH_LEASE'], str(self.window / 'private-lease.json'))
            self.assertNotIn('AMIGA_LAUNCH_INFLIGHT', kw['env'])
            return Child()
        self.supervisor = SimpleNamespace(guard=self.d.guard, decode=json.loads, popen=popen, clock=time.monotonic)

    def test_one_child_uses_new_parent_original_lease_and_zero_input_scope(self):
        result = dispatch.execute(self.parent_ref, self.supervisor)
        intent = json.loads((self.window / 'capture-dispatch-intent.json').read_text())
        self.assertEqual(result['state'], 'CONTEXT_V3_PARENT_RETURNED_LIFECYCLE_PENDING')
        self.assertEqual(result['returncode'], 0); self.assertFalse(result['guardReleaseClaimed'])
        for key in ('buttonFreeMovesMaximum', 'leftClicksMaximum', 'hostRaisesMaximum', 'probeStagesMaximum'):
            self.assertEqual(intent[key], 0)
        with self.assertRaises(RuntimeError): dispatch.execute(self.parent_ref, self.supervisor)
        self.assertEqual(len(self.children), 1)

    def test_nonzero_child_is_retained_without_retry_or_release(self):
        self.option = 'nonzero'; result = dispatch.execute(self.parent_ref, self.supervisor)
        self.assertEqual(result['returncode'], 3); self.assertEqual(result['signalsSent'], 0)
        self.assertEqual(self.release_calls, 0)

    def test_timeout_retains_child_without_signal_or_replay(self):
        self.option = 'timeout'
        with self.assertRaises(subprocess.TimeoutExpired): dispatch.execute(self.parent_ref, self.supervisor)
        self.assertTrue((self.window / 'capture-supervisor-timeout.json').exists())
        self.assertTrue((self.window / 'capture-supervisor-failure.json').exists())
        self.assertFalse((self.window / 'capture-dispatch-result.json').exists())
        with self.assertRaises(RuntimeError): dispatch.execute(self.parent_ref, self.supervisor)
        self.assertEqual(len(self.children), 1)

    def test_wait_failure_retains_child_without_signal(self):
        self.option = 'wait-error'
        with self.assertRaises(RuntimeError): dispatch.execute(self.parent_ref, self.supervisor)
        failure = json.loads((self.window / 'capture-supervisor-failure.json').read_text())
        self.assertEqual(failure['pid'], 888888); self.assertEqual(failure['signalsSent'], 0)

    def test_changed_owner_never_spawns(self):
        self.condition = 'owner'
        with self.assertRaises(RuntimeError): dispatch.execute(self.parent_ref, self.supervisor)
        self.assertEqual(self.children, [])

    def test_hold_never_spawns(self):
        self.phase = 'HOLD'
        with self.assertRaises(RuntimeError): dispatch.execute(self.parent_ref, self.supervisor)
        self.assertEqual(self.children, [])

    def test_existing_case_evidence_is_not_dispatched(self):
        Path(self.b['outputDirectory']).mkdir()
        with self.assertRaises(RuntimeError): dispatch.execute(self.parent_ref, self.supervisor)
        self.assertFalse((self.window / 'capture-dispatch-intent.json').exists())
        self.assertEqual(self.children, [])

    def test_inherited_authority_never_spawns(self):
        for key in dispatch.INHERITED_AUTHORITY:
            with self.subTest(key=key), patch.dict(dispatch.os.environ, {key: 'HOST_FOREIGN'}):
                with self.assertRaises(RuntimeError): dispatch.execute(self.parent_ref, self.supervisor)
                self.assertEqual(self.children, [])


if __name__ == '__main__': unittest.main()
