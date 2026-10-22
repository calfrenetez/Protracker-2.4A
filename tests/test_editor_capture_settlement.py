"""Real private files/locks; fake original guard, service and restore adapter."""
from contextlib import contextmanager
import json
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
from types import SimpleNamespace
import types
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import editor_pointer_parent as parent
import editor_probe_stopped as stopped
import editor_capture_namespace as app_ns
import editor_capture_settlement as full
from editor_probe_namespace import save
import test_editor_probe_stopped as stopped_tests


class FullSettlementTests(unittest.TestCase):
    def setUp(self):
        stopped_tests.StoppedProbeTests.setUp(self)
        for module in (full, app_ns):
            p = patch.object(module, 'HARNESS', self.h); p.start(); self.addCleanup(p.stop)
        self.app = self.root / 'app-data'; self.app.mkdir()
        p = patch.object(full, 'APP_ROOT', self.app); p.start(); self.addCleanup(p.stop)
        (self.h / 'runtime/launcher-transport.lock').touch()
        self.session = self.root / 'source'; self.session.mkdir(); self.roles = {}
        self.restore_calls = 0; self.release_calls = 0; self.release_error = False; self.restore_error = False
        self.released = False; self.protection_error = False
        for name in ('canonicalProfile', 'emulatorState', 'bridgeEnabled'):
            path = self.h / 'runtime' / name; path.write_bytes(('ORIGINAL ' + name).encode())
            ref = parent.read(path)[1]; self.roles[name] = path
            save(self.window / ('before-' + name + '.ref.json'), ref)
            (self.window / ('before-' + name + '.bin')).write_bytes(path.read_bytes())
        scope = {n: json.loads((self.window / ('before-' + n + '.ref.json')).read_text()) for n in self.roles}
        self.role_ref = save(self.window / 'before-owned-lifecycle-files.json', scope)
        for path in self.roles.values(): path.write_bytes(b'OWN SESSION BODY')
        rows = {}
        for name in sorted(full.THEMES) + ['Data/file%02d' % i for i in range(29)]:
            path = self.app / name; path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(name.encode())
            backup = self.session / 'saved-app-data' / name; backup.parent.mkdir(parents=True, exist_ok=True)
            backup.write_bytes(path.read_bytes())
            rows[name] = {'original': parent.read(path)[1], 'backup': str(backup.relative_to(self.session))}
        self.rows = rows
        self.app_ref = save(self.window / 'current-app-data-before-and-backups.json', {'files': rows})
        for name in full.THEMES:
            path = self.app / name; old = rows[name]['original']['fullFacts']
            os.utime(path, ns=(path.stat().st_atime_ns, old['st_mtime_ns'] + 1000000))
        self.preserve_ref = save(self.window / 'fresh-preservation-before.json', {'FAKE_HOST_BASELINE': True})
        alias = self.root / 'volume/PT24GEdit'; alias.parent.mkdir(); alias.write_bytes(b'HOST editor')
        self.parent['blankBinding'].update(mountedCandidate=parent.read(alias)[1],
            outputDirectory=str(self.window / 'blank-capture-full-fixture-r1'))
        self.parent['blankBinding']['modulePaths']['capture_runner'] = str(self.session / 'capture_runner.py')
        self.parent['runLabel'] = 'blank-capture-full-fixture-r1'
        run = self.h / 'runtime/Dev/Tests' / self.parent['runLabel']; run.mkdir(); (run / 'copies').mkdir()
        (run / 'editor.log').write_bytes(b'HOST retained log'); (run / 'copies/saved').write_bytes(b'HOST saved')
        (run.parent / ('launch-' + run.name)).write_bytes(b'HOST launcher')
        self.run = run
        sources = self.parent['sourcePins']
        for module in (full, app_ns): sources[str(Path(module.__file__))] = parent.read(module.__file__)[1]
        freeze_path = Path(self.parent['sourceFreezeRef']['path'])
        freeze_path.write_text(json.dumps({'state': parent.FROZEN_STATE, 'sources': sources}))
        self.parent['sourceFreezeRef'] = parent.read(freeze_path)[1]
        self.rebind_parent()
        self.full_b = {'state': full.STATE, 'acceptedByRoot': True, 'ownerRunID': self.owner,
            'windowDirectory': str(self.window), 'mode': 'normal', 'allowedActions': dict(full.ALLOWED),
            'limits': {'whole': 180}, 'stoppedBindingRef': save(self.window / 'probe-stopped-binding.json', self.b),
            'appDataBeforeRef': self.app_ref, 'roleBaselineRef': self.role_ref,
            'preservationBeforeRef': self.preserve_ref}
        outer = self; base_guard = self.d.guard
        class Guard(base_guard):
            def status(self):
                value = super().status()
                if outer.released: value.update(phase='EMPTY', holds=[], reservation=None)
                else: value['reservation'] = {'lease': {'run_id': outer.owner}}
                return value
            def release(self, lease, proof):
                outer.release_calls += 1
                if outer.release_error: raise RuntimeError('FAKE original release refusal')
                outer.assertFalse(outer.run.exists()); outer.assertFalse(outer.directory.exists())
                cleanup = json.loads((outer.window / 'cleanup-release-fact.json').read_text())
                outer.assertEqual(cleanup['probeNamespaceCleanup']['facts']['mode'], outer.full_b['mode'])
                outer.assertTrue(cleanup['appNamespace']['appNamespacePreservedAndAbsent'])
                outer.assertEqual(set(proof['records']), {'authorization', 'restoration', 'absence', 'cleanup'})
                outer.released = True
                path = outer.h / 'runtime/launcher-guard/history'; path.mkdir()
                (path / 'HOST-archive.json').write_text(json.dumps({'proof': proof,
                    'reservation': {'lease': lease}, 'holds': [{}] if outer.phase == 'HOLD' else []}))
                return {'released': True, 'fence': 8, 'archive': 'HOST-archive.json'}
        @contextmanager
        def locks():
            with parent.command_lock(self.h / 'runtime/test.lock'), parent.command_lock(self.h / 'runtime/launcher-transport.lock'):
                yield
        def resident(path, private=False): return parent.read(path)
        def admissions(path, action): return json.loads(parent.read(path)[0]), parent.read(path)[1], self.window
        def restore(a, ap, window, action):
            self.restore_calls += 1; restored = {}
            with locks():
                for role, path in self.roles.items():
                    before = (self.window / ('before-' + role + '.bin')).read_bytes()
                    path.write_bytes(before); os.utime(path, ns=(path.stat().st_atime_ns, scope[role]['fullFacts']['st_mtime_ns']))
                    restored[role] = parent.read(path)[1]
                    if self.restore_error: raise RuntimeError('FAKE partial original restore')
                result = self.window / 'restore-canonical-profile-session-result.json'
                save(result, {'state': 'THREE_HOST_FILE_ORIGINAL_BODIES_RESTORED_AFTER_VERIFIED_OWNED_STOP',
                              'restoredFull4s': restored})
                return result
        def preserve(before):
            if self.protection_error: raise RuntimeError('FAKE protected source drift')
            self.assertEqual(before, {'FAKE_HOST_BASELINE': True})
            return {'protectedPaths': 68, 'rawMetadataCompared': True}
        self.d.guard = Guard; self.d.decode = json.loads
        self.d.files = SimpleNamespace(ROLES=self.roles, locks=locks, resident=resident,
            pinned=lambda ref, private=False: parent.pinned(ref), admissions=admissions, execute=restore)
        self.d.preservation = SimpleNamespace(verify_current=preserve)

    def rebind_parent(self):
        path = self.window / 'mouse-parent-staged-binding.json'; path.write_text(json.dumps(self.parent))
        self.b['parentBindingRef'] = parent.read(path)[1]

    def settle(self): return full.execute(self.full_b, self.d)

    def test_normal_full_path_preserves_probe_app_then_restores_all_files_and_releases(self):
        result = self.settle()
        self.assertEqual(result['state'], 'FULL_STOPPED_EDITOR_AND_PROBE_RESTORED_AND_RELEASED')
        self.assertEqual((self.restore_calls, self.release_calls), (1, 1))
        self.assertEqual(result['appDataFiles'], 31)
        self.assertFalse(result['nativeCleanExitClaimed']); self.assertFalse(result['runtimeAcceptance'])
        self.assertEqual((self.window / 'preserved-guest-namespace/run/editor.log').read_bytes(), b'HOST retained log')
        self.assertEqual(full.verify_app_data(self.rows, full.THEMES).keys(), self.rows.keys())

    def test_failed_full_path_preserves_hold_until_one_original_public_release(self):
        self.phase = 'HOLD'; self.b['mode'] = self.full_b['mode'] = 'failed'
        path = Path(self.full_b['stoppedBindingRef']['path']); path.write_text(json.dumps(self.b))
        self.full_b['stoppedBindingRef'] = parent.read(path)[1]
        result = self.settle(); self.assertEqual(result['archivedHolds'], 1)
        self.assertEqual(self.release_calls, 1)

    def test_saved_backup_body_change_refuses_before_any_namespace_mutation(self):
        row = self.rows[next(iter(self.rows))]; (self.session / row['backup']).write_bytes(b'changed')
        with self.assertRaises(RuntimeError): self.settle()
        self.assertTrue(self.directory.exists()); self.assertTrue(self.run.exists())
        self.assertEqual((self.restore_calls, self.release_calls), (0, 0))

    def test_changed_app_body_preserves_namespaces_but_never_restores_or_releases(self):
        (self.app / 'Data/file00').write_bytes(b'changed')
        with self.assertRaises(RuntimeError): self.settle()
        self.assertTrue((self.window / 'namespace-cleanup-result.json').exists())
        self.assertTrue((self.window / 'probe-cleanup-fact.json').exists())
        self.assertEqual((self.restore_calls, self.release_calls), (0, 0))

    def test_unexpected_input_volume_content_preserves_app_namespace(self):
        alias = Path(self.parent['blankBinding']['mountedCandidate']['path'])
        (alias.parent / 'OTHER').write_bytes(b'unrelated')
        with self.assertRaises(RuntimeError): self.settle()
        self.assertTrue(self.run.exists()); self.assertEqual(self.release_calls, 0)

    def test_symlink_app_namespace_leaf_is_not_removed(self):
        (self.run / 'link').symlink_to(self.app / 'Data/file00')
        with self.assertRaises(RuntimeError): self.settle()
        self.assertTrue((self.run / 'link').is_symlink()); self.assertEqual(self.release_calls, 0)

    def test_partial_original_restore_is_retained_and_not_replayed(self):
        self.restore_error = True
        with self.assertRaises(RuntimeError): self.settle()
        self.assertTrue((self.window / 'full-settlement-failed.json').exists())
        with self.assertRaises(RuntimeError): self.settle()
        self.assertEqual((self.restore_calls, self.release_calls), (1, 0))

    def test_protected_source_drift_prevents_release_after_restoration(self):
        self.protection_error = True
        with self.assertRaises(RuntimeError): self.settle()
        self.assertEqual((self.restore_calls, self.release_calls), (1, 0))
        self.assertFalse((self.window / 'release-proof.json').exists())

    def test_preserved_app_copy_drift_is_not_adopted_into_release_facts(self):
        def changed(before):
            (self.window / 'preserved-guest-namespace/run/editor.log').write_bytes(b'changed')
            return {'protectedPaths': 68}
        self.d.preservation.verify_current = changed
        with self.assertRaises(RuntimeError): self.settle()
        self.assertEqual(self.restore_calls, 1); self.assertEqual(self.release_calls, 0)

    def test_changed_guard_or_live_target_refuses_before_either_namespace(self):
        self.condition = 'vm'
        with self.assertRaises(RuntimeError): self.settle()
        self.assertTrue(self.run.exists()); self.assertTrue(self.directory.exists())
        self.assertEqual(self.restore_calls, 0)

    def test_original_release_refusal_never_replays_or_clears_hold(self):
        self.phase = 'HOLD'; self.b['mode'] = self.full_b['mode'] = 'failed'
        path = Path(self.full_b['stoppedBindingRef']['path']); path.write_text(json.dumps(self.b))
        self.full_b['stoppedBindingRef'] = parent.read(path)[1]; self.release_error = True
        with self.assertRaises(RuntimeError): self.settle()
        with self.assertRaises(RuntimeError): self.settle()
        self.assertEqual(self.release_calls, 1); self.assertFalse(self.released)

    def test_inventory_change_or_unapproved_mtime_is_not_restored(self):
        path = self.app / 'Data/file00'; old = path.stat()
        os.utime(path, ns=(old.st_atime_ns, old.st_mtime_ns + 1000000))
        with self.assertRaises(RuntimeError): self.settle()
        self.assertEqual(self.restore_calls, 0)

    def test_probe_supplement_borrows_existing_lock_without_closing_it(self):
        fact = stopped.execute(self.b, self.d)
        with parent.command_lock(self.h / 'runtime/test.lock') as fd:
            stopped.release_cleanup_supplement(self.b, fact, self.d, lock_fd=fd)
            self.assertEqual(os.fstat(fd).st_ino, (self.h / 'runtime/test.lock').stat().st_ino)
            with self.assertRaises(BlockingIOError):
                with parent.command_lock(self.h / 'runtime/test.lock'): pass

    def test_actual_alternate_runtime_refuses_before_settlement_imports(self):
        child = subprocess.run(['/usr/bin/python3', '-B', full.__file__, '--help'],
                               capture_output=True, text=True, timeout=20)
        self.assertNotEqual(child.returncode, 0); self.assertEqual(child.stdout, '')
        self.assertIn('Original harness clock required before settlement imports', child.stderr)
        self.assertNotIn('TypeError', child.stderr)

    def real_originals(self, mode):
        """Actual unchanged helper and public guard, exclusively in this temp root."""
        installed = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra/scripts/launcher_guard.py')
        helper = Path(__file__).resolve().parents[3] / 'outputs/ProTracker-2.4G-editor-firstmouse-postmove-refusal-20261022-r3/frozen-source/canonical_profile_session_files.py'
        if not installed.is_file() or not helper.is_file(): self.skipTest('Installed original HOST integration sources unavailable')
        body = helper.read_bytes(); self.assertEqual(parent.digest(body), full.ORIGINAL_HELPER)
        spec = importlib.util.spec_from_file_location('pt24g_original_guard_HOST_fixture', installed)
        original = importlib.util.module_from_spec(spec); spec.loader.exec_module(original)
        state = self.h / 'runtime/launcher-guard'; (state / 'state.json').unlink(); state.rmdir()
        guard = original.initialize(self.h, [], {'mode': 'temporary-fixture', 'approved': True,
            'legacy_holds_verified': True, 'record_sha256': '1' * 64})
        lease = guard.reserve('amiberry-030', 'HOST fixture', 'No target; full settlement integration')
        if mode == 'failed': guard.hold(lease, 'HOST synthetic failed case; no target ever contacted')
        old_probe = self.directory / 'PTEditorPointerV2'; old_probe.unlink(); self.directory.rmdir()
        (self.window / 'passive-probe-stage').rename(self.window / 'old-HOST-stage')
        self.owner = lease['run_id']; self.parent['ownerRunID'] = self.owner
        probe = self.parent['passiveProbe']; probe['ownerRunID'] = self.owner
        import editor_probe_namespace as ns
        with parent.command_lock(self.h / 'runtime/test.lock') as fd:
            stage_ref = ns.stage(probe, self.window / 'passive-probe-stage',
                SimpleNamespace(stage_owner=lambda owner: self.assertEqual(owner, self.owner)), lock_fd=fd)
        probe['namespaceStageRef'] = stage_ref
        self.b.update(ownerRunID=self.owner, mode=mode,
            guardRef=parent.read(state / 'state.json')[1])
        stop_path = Path(self.b['stopReceiptRef']['path'])
        stop_path.write_text(json.dumps({'ownerRunID': self.owner, 'pid': 999999, 'processAndGroupAbsent': True}))
        self.b['stopReceiptRef'] = parent.read(stop_path)[1]
        self.rebind_parent()
        sb = Path(self.full_b['stoppedBindingRef']['path']); sb.write_text(json.dumps(self.b))
        self.full_b.update(ownerRunID=self.owner, mode=mode, stoppedBindingRef=parent.read(sb)[1])
        self.d.guard = original.LauncherGuard; self.d.lease_input = lambda *a: lease
        self.d.decode = original._decode
        # Configure only fixture paths and synthetic original/session bytes.
        # All original saved-file function bodies execute unchanged.
        f = types.ModuleType('pt24g_original_saved_files_HOST_fixture'); f.__file__ = str(helper)
        exec(compile(body, str(helper), 'exec'), f.__dict__)
        f.I = self.h; f.ROLES = self.roles
        self.window.chmod(0o700); (self.h / 'runtime/launcher-transport.lock').chmod(0o600)
        scope = json.loads(parent.pinned(self.role_ref)); before_profile = b'HOST original profile\n'
        for role, path in self.roles.items():
            backup = self.window / ('before-' + role + '.bin')
            if role == 'canonicalProfile':
                backup.write_bytes(before_profile); path.write_bytes(before_profile)
                scope[role] = parent.read(path)[1]
                path.write_bytes(before_profile + b'HOST suffix\n')
            backup.chmod(0o600); scope[role]['absent'] = False
        baseline = Path(self.role_ref['path']); baseline.write_text(json.dumps(scope))
        self.full_b['roleBaselineRef'] = parent.read(baseline)[1]
        save(self.window / 'patch-canonical-profile-session-result.json', {
            'state': 'CANONICAL_PROFILE_SESSION_APPEND_APPLIED_ORIGINAL_BYTES_PRESERVED'})
        f.ORIGINAL = parent.digest(before_profile); f.APPEND = b'HOST suffix\n'
        f.SESSION = parent.digest(before_profile + f.APPEND)
        self.d.files = f
        return guard

    def test_real_original_saved_files_and_guard_release_in_private_normal_fixture(self):
        guard = self.real_originals('normal'); result = self.settle()
        self.assertEqual(result['state'], 'FULL_STOPPED_EDITOR_AND_PROBE_RESTORED_AND_RELEASED')
        self.assertEqual(guard.status()['phase'], 'EMPTY'); self.assertEqual(result['archivedHolds'], 0)

    def test_real_original_saved_files_and_guard_archive_hold_in_private_failed_fixture(self):
        guard = self.real_originals('failed'); result = self.settle()
        self.assertEqual(guard.status()['phase'], 'EMPTY'); self.assertEqual(result['archivedHolds'], 1)


if __name__ == '__main__': unittest.main()
