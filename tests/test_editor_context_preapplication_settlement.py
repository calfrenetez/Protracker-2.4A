"""Private original guard/restorer integration; synthetic process/bridge observations."""
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import editor_context_preapplication_settlement as cleanup
import editor_context_settlement as full
import editor_pointer_parent as original
import test_editor_context_lifecycle as fixtures
import editor_context_volume as volume


class PreapplicationSettlementTests(unittest.TestCase):
    setUp = fixtures.ContextLifecycleTests.setUp
    prepare = fixtures.ContextLifecycleTests.prepare
    full_binding = fixtures.ContextLifecycleTests.full_binding
    real_originals = fixtures.ContextLifecycleTests.real_originals

    def binding(self, real=False):
        if real: b, self.guard = self.real_originals('failed')
        else: b = self.full_binding('failed')
        # Synthetic fixture only. The real actor never removes an app namespace.
        (self.run / 'editor.log').unlink(); (self.run / 'copies').rmdir(); self.run.rmdir()
        (self.run.parent / ('launch-' + self.label)).unlink()
        p = patch.object(cleanup, 'HARNESS', self.h); p.start(); self.addCleanup(p.stop)
        b.update(state=cleanup.STATE, allowedActions=dict(cleanup.ALLOWED))
        b['independentCleanupSourceRef'] = original.read(cleanup.__file__)[1]
        b['independentHostQualificationRef'] = volume.save(self.root / 'HOST-only-qualification.json', {
            'state': 'PREAPPLICATION_RESTORATION_HOST_PASSED_NOT_RUNTIME',
            'sourceRef': b['independentCleanupSourceRef'], 'returncode': 0, 'testsPassed': 1,
            'HOST_SYNTHETIC': True})
        b['failureRef'] = volume.save(self.window / 'parent-pre-operation-failure.json', {
            'state': cleanup.FAILURE, 'ownerRunID': b['ownerRunID'],
            'actualOwnedProcess': self.owned, 'applicationLaunches': 0,
            'passiveNativeExecutions': 0, 'guiInputEvents': 0, 'retryAllowed': False})
        b['dispatchResultRef'] = volume.save(self.window / 'capture-dispatch-result.json', {
            'state': 'CONTEXT_V3_PARENT_RETURNED_LIFECYCLE_PENDING', 'returncode': 1,
            'signalsSent': 0, 'guardReleaseClaimed': False, 'retryAllowed': False})
        b['stderrRef'] = volume.put(self.window / 'capture.stderr',
            b"HOST synthetic trace original.pinned(p['activeProfile'])\nRuntimeError: Actual pinned input changed\n")
        b['stdoutRef'] = volume.put(self.window / 'capture.stdout', b'')
        return b

    def assert_clean(self, result):
        self.assertEqual(result['state'], 'PREAPPLICATION_V3_SAVED_FILES_RESTORED_AND_RELEASED')
        self.assertEqual((result['archivedHolds'], result['restoredRoles'], result['appDataFiles']), (1, 3, 31))
        self.assertFalse(result['runtimeAcceptance']); self.assertFalse(result['nativeCleanExitClaimed'])
        for path in (self.run, self.out, self.run.parent / ('launch-' + self.label)): self.assertFalse(path.exists())
        value = json.loads((self.window / 'cleanup-release-fact.json').read_text())
        absence = value['preapplicationNamespaceAbsence']['facts']
        self.assertEqual((absence['filesRemoved'], absence['directoriesRemoved']), (0, 0))
        self.assertNotIn('appNamespace', value)
        self.assertEqual(original.pinned(self.p['stagedCandidate']), self.native)
        self.assertFalse((self.window / 'namespace-cleanup-intent.json').exists())

    def test_private_hold_restores_releases_once_without_app_namespace(self):
        b = self.binding(); self.assert_clean(cleanup.execute(b, self.d))
        self.assertEqual((self.restore_calls, self.release_calls), (1, 1))
        with self.assertRaises(RuntimeError): cleanup.execute(b, self.d)
        self.assertEqual((self.restore_calls, self.release_calls), (1, 1))

    def test_original_guard_and_saved_file_algorithm_private_hold_release(self):
        b = self.binding(real=True); self.assert_clean(cleanup.execute(b, self.d))
        self.assertEqual(self.guard.status()['phase'], 'EMPTY')
        self.assertEqual(self.roles['canonicalProfile'].read_bytes(), self.original)

    def test_profile_ctime_discontinuity_restores_without_requalifying_runtime(self):
        b = self.binding(real=True); path = self.roles['canonicalProfile']; before = original.read(path)[1]
        path.chmod(path.stat().st_mode & 0o777); after = original.read(path)[1]
        self.assertEqual(before['sha256'], after['sha256'])
        self.assertNotEqual(before['fullFacts']['st_ctime_ns'], after['fullFacts']['st_ctime_ns'])
        self.assert_clean(cleanup.execute(b, self.d))
        original.pinned(b['failureRef'])

    def test_full_actor_refuses_distinct_independent_state(self):
        b = self.binding()
        with self.assertRaises(RuntimeError): full.execute(b, self.d)
        self.assertEqual((self.restore_calls, self.release_calls), (0, 0))

    def test_current_owner_service_process_socket_port_bridge_required(self):
        b = self.binding()
        for condition in ('owner', 'service', 'vm', 'group', 'socket', 'port', 'bridge'):
            with self.subTest(condition=condition):
                self.condition = condition
                with self.assertRaises(RuntimeError): cleanup.execute(b, self.d)
                self.assertFalse((self.window / 'preapplication-settlement-intent.json').exists())
                self.assertEqual((self.restore_calls, self.release_calls), (0, 0))

    def test_any_app_output_presence_refuses_without_deletion(self):
        b = self.binding()
        for path in (self.run, self.out, self.run.parent / ('launch-' + self.label)):
            path.write_bytes(b'foreign')
            with self.assertRaises(RuntimeError): cleanup.execute(b, self.d)
            self.assertEqual(path.read_bytes(), b'foreign'); path.unlink()
        self.assertEqual((self.restore_calls, self.release_calls), (0, 0))
        self.assertFalse((self.window / 'probe-stopped-intent.json').exists())

    def test_dangling_symlink_counts_as_presence(self):
        b = self.binding(); self.run.symlink_to('/private/tmp/pt24g-HOST-absent-destination')
        with self.assertRaises(RuntimeError): cleanup.execute(b, self.d)
        self.assertTrue(self.run.is_symlink()); self.assertEqual(self.restore_calls, 0)

    def test_application_probe_or_input_count_prevents_preapplication_admission(self):
        b = self.binding(); path = Path(b['failureRef']['path']); value = json.loads(path.read_text())
        for name in ('applicationLaunches', 'passiveNativeExecutions', 'guiInputEvents'):
            altered = dict(value); altered[name] = 1; path.write_text(json.dumps(altered)); b['failureRef'] = original.read(path)[1]
            with self.assertRaises(RuntimeError): cleanup.execute(b, self.d)
        self.assertEqual((self.restore_calls, self.release_calls), (0, 0))

    def test_host_qualification_must_bind_exact_cleanup_source(self):
        b = self.binding(); path = Path(b['independentHostQualificationRef']['path']); q = json.loads(path.read_text())
        q['sourceRef']['sha256'] = '0' * 64; path.write_text(json.dumps(q)); b['independentHostQualificationRef'] = original.read(path)[1]
        with self.assertRaises(RuntimeError): cleanup.execute(b, self.d)
        self.assertFalse((self.window / 'preapplication-settlement-intent.json').exists())

    def test_failed_runtime_evidence_drift_refuses_before_mutation(self):
        b = self.binding()
        for key in ('failureRef', 'dispatchResultRef', 'stderrRef', 'stdoutRef'):
            path = Path(b[key]['path']); body = path.read_bytes(); path.write_bytes(body + b'changed')
            with self.assertRaises(RuntimeError): cleanup.execute(b, self.d)
            path.write_bytes(body); b[key] = original.read(path)[1]
        self.assertEqual((self.restore_calls, self.release_calls), (0, 0))

    def test_unrelated_settings_body_is_not_recovered(self):
        b = self.binding(); path = full.APP_ROOT / 'Data/file00'; path.write_bytes(b'unrelated')
        with self.assertRaises(RuntimeError): cleanup.execute(b, self.d)
        self.assertEqual(path.read_bytes(), b'unrelated'); self.assertEqual(self.restore_calls, 0)
        self.assertFalse((self.window / 'probe-stopped-intent.json').exists())

    def test_namespace_appearance_after_restoration_blocks_release_without_replay(self):
        b = self.binding()
        def drift(before):
            self.run.mkdir(); return {'HOST_ONLY': True}
        self.d.preservation.verify_current = drift
        with self.assertRaises(RuntimeError): cleanup.execute(b, self.d)
        self.assertTrue(self.run.exists()); self.assertEqual((self.restore_calls, self.release_calls), (1, 0))
        self.assertTrue((self.window / 'preapplication-settlement-failed.json').exists())
        with self.assertRaises(RuntimeError): cleanup.execute(b, self.d)
        self.assertEqual((self.restore_calls, self.release_calls), (1, 0))

    def test_retained_v3_input_drift_blocks_release(self):
        b = self.binding()
        def drift(before):
            (self.window / 'preserved-readonly-context-input/PTEditorPointerContextV3').write_bytes(b'foreign')
            return {'HOST_ONLY': True}
        self.d.preservation.verify_current = drift
        with self.assertRaises(RuntimeError): cleanup.execute(b, self.d)
        self.assertEqual((self.restore_calls, self.release_calls), (1, 0))

    def test_never_replays_existing_full_restore_or_release_transaction(self):
        b = self.binding()
        for name in ('full-settlement-intent.json', 'restore-admission.json', 'release-proof.json', 'preapplication-settlement-failed.json'):
            path = self.window / name; path.write_bytes(b'HOST previous attempt')
            with self.assertRaises(RuntimeError): cleanup.execute(b, self.d)
            path.unlink()
        self.assertEqual((self.restore_calls, self.release_calls), (0, 0))


if __name__ == '__main__': unittest.main()
