"""Private host inputs only. No installed service, guest or canonical writes."""
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import uuid

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import editor_capture_volume as capture
import editor_probe_volume as volume


class ProbeVolumeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='pt24g-probe-volume-HOST-', dir='/private/tmp')
        self.addCleanup(self.temp.cleanup); self.root = Path(self.temp.name)
        label = 'host-' + uuid.uuid4().hex
        self.capture = Path('/private/tmp/pt24g-editor-capture-volume-' + label)
        self.destination = Path('/private/tmp/pt24g-editor-probe-volume-' + label)
        self.evidence = Path('/private/tmp/pt24g-editor-probe-preparation-' + label)
        self.capture.mkdir(mode=0o700)
        def cleanup():
            for p in (self.destination, self.capture, self.evidence):
                if p.is_dir() and not p.is_symlink():
                    p.chmod(0o700); shutil.rmtree(p)
                elif os.path.lexists(p): p.unlink()
        self.addCleanup(cleanup)
        prefix = (b'# HOST SYNTHETIC PROFILE\n'
                  b'filesystem2=rw,DH2:Dev:/host-fixture/Dev,-128\n'
                  b'uaehf1=dir,rw,DH2:Dev:/host-fixture/Dev,-128\n')
        self.original = prefix + b'#' * (4828 - len(prefix) - 1) + b'\n'
        self.native = b'HOST synthetic V2'; source = b'HOST synthetic source'; editor = b'HOST synthetic editor'
        for module, name, value in ((capture, 'ORIGINAL_PROFILE_SHA256', hashlib.sha256(self.original).hexdigest()),
                                    (volume, 'NATIVE', volume.digest(self.native)),
                                    (volume, 'SOURCE', volume.digest(source)),
                                    (volume, 'EDITOR', volume.digest(editor))):
            p = patch.object(module, name, value); p.start(); self.addCleanup(p.stop)
        original = volume.put(self.root / 'profile-before', self.original)
        native = volume.put(self.root / 'native', self.native)
        source_ref = volume.put(self.root / 'source.c', source)
        editor_ref = volume.put(self.root / 'editor', editor)
        alias = volume.put(self.capture / 'PT24GEdit', editor)
        suffix = capture.profile_append(self.original, str(self.capture))
        prep = volume.save(self.root / 'capture-preparation.json', {
            'state': 'PRIVATE_LOCAL_CANDIDATE_VOLUME_AND_ADDITIVE_PROFILE_SUFFIX_PREPARED_NOT_ADMITTED_OR_MOUNTED',
            'targetOperations': 0, 'canonicalProfileWrites': 0, 'privateCandidate': alias,
            'originalCandidate': editor_ref, 'guestCandidate': 'PTCapture:PT24GEdit',
            'appendASCII': suffix.decode('ascii'), 'proposedProfile': {
                'bytes': len(self.original + suffix), 'sha256': volume.digest(self.original + suffix)[1]}})
        receipt = volume.save(self.root / 'compile.json', {
            'state': 'PASSIVE_EDITOR_V2_SOURCE_AND_NATIVE_COMPILE_HOST_CHECKED_NOT_EXECUTED',
            'binary': {'bytes': len(self.native), 'sha256': volume.digest(self.native)[1]},
            'sourcePinsEqualBeforeAfter': {'tools/diagnostics/native_editor_pointer_snapshot.c': {
                'bytes': len(source), 'sha256': volume.digest(source)[1]}}})
        self.refs = dict(originalProfile=original, capturePreparation=prep, nativeSource=source_ref,
                         nativeCandidate=native, compileReceipt=receipt)

    def prepare(self): return volume.prepare(self.refs, self.destination, self.evidence)

    def test_profile_retains_editor_suffix_and_adds_only_paired_readonly_probe(self):
        editor = capture.profile_append(self.original, str(self.capture))
        suffix = volume.profile_append(self.original, str(self.capture), str(self.destination))
        self.assertTrue(suffix.startswith(editor))
        self.assertEqual(suffix[len(editor):], (
            'filesystem2=ro,DH4:PTProbe:' + str(self.destination) + ',-128\n'
            'uaehf3=dir,ro,DH4:PTProbe:' + str(self.destination) + ',-128\n').encode())
        self.assertNotIn(b'rw,DH4:', suffix)
        self.assertTrue((self.original + suffix).startswith(self.original))

    def test_existing_slots_or_changed_original_profile_refuse(self):
        for slot in (b'PTProbe', b'DH4:', b'uaehf3='):
            original = self.original.replace(b'# HOST', slot + b' HOST', 1)
            original = original[:4827] + b'\n'
            with patch.object(capture, 'ORIGINAL_PROFILE_SHA256', hashlib.sha256(original).hexdigest()):
                with self.assertRaises(ValueError):
                    volume.profile_append(original, str(self.capture), str(self.destination))
        with self.assertRaises(ValueError):
            volume.profile_append(b'changed', str(self.capture), str(self.destination))

    def test_nonlocal_ambiguous_or_injected_probe_path_refuses(self):
        for path in ('/tmp/pt24g-editor-probe-volume-a', '/private/tmp/other',
                     str(self.destination) + '/nested', str(self.destination) + ',x',
                     str(self.destination) + '\nuaehf0=bad', str(self.destination) + ' space', None):
            with self.subTest(path=path), self.assertRaises(ValueError):
                volume.profile_append(self.original, str(self.capture), path)

    def test_preparation_keeps_editor_input_exact_and_returns_no_admission(self):
        before = volume.read(self.capture / 'PT24GEdit')[1]
        record = volume.verify(self.prepare())
        self.assertEqual(volume.read(self.capture / 'PT24GEdit')[1], before)
        self.assertEqual([p.name for p in self.capture.iterdir()], ['PT24GEdit'])
        self.assertEqual([p.name for p in self.destination.iterdir()], ['PTEditorPointerV2'])
        self.assertEqual(volume.pinned(record['candidate']), self.native)
        self.assertEqual(stat.S_IMODE(self.destination.stat().st_mode), 0o500)
        self.assertEqual(stat.S_IMODE((self.destination / 'PTEditorPointerV2').stat().st_mode), 0o500)
        self.assertFalse(record['runtimeAdmission']); self.assertFalse(record['sourceFreezeCreated'])
        self.assertEqual((record['targetOperations'], record['canonicalProfileWrites']), (0, 0))
        self.assertEqual(volume.pinned(self.refs['originalProfile']), self.original)

    def test_replay_refuses_without_input_or_receipt_change(self):
        ref = self.prepare(); before = volume.pinned(ref)
        with self.assertRaises(RuntimeError): self.prepare()
        self.assertEqual(volume.pinned(ref), before); volume.verify(ref)

    def test_preparation_cannot_be_used_as_existing_runtime_admission(self):
        import editor_pointer_binding as binder
        import editor_pointer_parent as parent
        record = volume.verify(self.prepare())
        with self.assertRaisesRegex(RuntimeError, 'Distinct exact parent admission'):
            parent.validate_binding(record)
        with self.assertRaisesRegex(RuntimeError, 'Distinct actual-start binder scope'):
            binder.validate_actual(record)

    def saved_file_fixture(self):
        import editor_pointer_binding as binder
        helper = Path(__file__).resolve().parents[3] / 'outputs/ProTracker-2.4G-editor-firstmouse-postmove-refusal-20261022-r3/frozen-source/canonical_profile_session_files.py'
        if not helper.is_file(): self.skipTest('Qualified saved-file helper source unavailable')
        self.assertEqual(volume.digest(helper.read_bytes()), binder.SAVED_FILE_HELPER)
        f = SimpleNamespace(__file__=str(helper), I=binder.HARNESS, APPEND=b'UNTOUCHED',
            ROLES={'canonicalProfile': binder.HARNESS / 'runtime/amiberry/Configurations/A1200-030-DEV.uae',
                   'emulatorState': binder.HARNESS / 'runtime/emulator.json',
                   'bridgeEnabled': binder.HARNESS / 'runtime/Dev/Tools/bridge-enabled'})
        sources = {str(helper): volume.read(helper)[1], str(Path(volume.__file__)): volume.read(volume.__file__)[1]}
        return binder, f, sources

    def test_prospective_saved_file_configuration_uses_same_complete_suffix(self):
        ref = self.prepare(); binder, f, sources = self.saved_file_fixture()
        binder.configure_readonly_probe_saved_files(f, sources, self.refs['capturePreparation'],
                                                   ref, self.refs['originalProfile'])
        record = volume.verify(ref)
        self.assertEqual(f.APPEND.decode('ascii'), record['appendASCII'])
        self.assertEqual(f.SESSION, volume.digest(self.original + f.APPEND))
        self.assertIn(b'filesystem2=ro,DH4:PTProbe:', f.APPEND)
        self.assertEqual(volume.pinned(self.refs['originalProfile']), self.original)

    def test_missing_preparation_source_pin_refuses_before_helper_configuration(self):
        ref = self.prepare(); binder, f, sources = self.saved_file_fixture()
        del sources[str(Path(volume.__file__))]
        with self.assertRaises(RuntimeError):
            binder.configure_readonly_probe_saved_files(f, sources, self.refs['capturePreparation'],
                                                       ref, self.refs['originalProfile'])
        self.assertEqual(f.APPEND, b'UNTOUCHED')

    def test_different_editor_preparation_cannot_be_substituted(self):
        ref = self.prepare(); binder, f, sources = self.saved_file_fixture()
        other = volume.put(self.root / 'other-preparation.json', volume.pinned(self.refs['capturePreparation']))
        with self.assertRaises(RuntimeError):
            binder.configure_readonly_probe_saved_files(f, sources, other, ref, self.refs['originalProfile'])
        self.assertEqual(f.APPEND, b'UNTOUCHED')

    def test_existing_destination_or_evidence_never_overwritten(self):
        for path in (self.destination, self.evidence):
            path.mkdir(); (path / 'unrelated').write_bytes(b'preserve')
            with self.assertRaises(RuntimeError): self.prepare()
            self.assertEqual((path / 'unrelated').read_bytes(), b'preserve')
            shutil.rmtree(path)

    def test_mismatched_scope_or_symlink_refuses_without_any_copy(self):
        with self.assertRaises(RuntimeError):
            volume.prepare(self.refs, self.destination, str(self.evidence) + '-other')
        self.destination.symlink_to(self.root, target_is_directory=True)
        with self.assertRaises(RuntimeError): self.prepare()
        self.assertFalse(self.evidence.exists())

    def test_changed_native_compile_or_capture_refuses_before_creation(self):
        for name in ('nativeCandidate', 'compileReceipt', 'capturePreparation'):
            before = volume.read(self.refs[name]['path'])[0]
            Path(self.refs[name]['path']).write_bytes(b'changed')
            with self.assertRaises(RuntimeError): self.prepare()
            self.assertFalse(self.destination.exists()); self.assertFalse(self.evidence.exists())
            Path(self.refs[name]['path']).write_bytes(before)
            self.refs[name] = volume.read(self.refs[name]['path'])[1]

    def test_editor_volume_extra_content_refuses(self):
        (self.capture / 'OTHER').write_bytes(b'preserve')
        with self.assertRaises(RuntimeError): self.prepare()
        self.assertFalse(self.destination.exists())

    def test_failed_copy_is_retained_without_replay_or_cleanup(self):
        original = volume.put
        def fail(path, body, mode=0o600):
            if Path(path).parent == self.destination: raise OSError('HOST copy failure')
            return original(path, body, mode)
        with patch.object(volume, 'put', side_effect=fail):
            with self.assertRaises(OSError): self.prepare()
        result = json.loads((self.evidence / 'FAILED.json').read_text())
        self.assertEqual(result['candidateCopies'], 1); self.assertTrue(self.destination.exists())
        with self.assertRaises(RuntimeError): self.prepare()

    def test_changed_prepared_body_or_metadata_strictly_refuses(self):
        ref = self.prepare(); path = self.destination / 'PTEditorPointerV2'
        path.chmod(0o700)
        with self.assertRaises(RuntimeError): volume.verify(ref)
        self.assertEqual(path.read_bytes(), self.native)

    def test_wrong_new_copy_permissions_retain_failure_without_completed_receipt(self):
        original = volume.put
        def restricted(path, body, mode=0o600):
            return original(path, body, 0o400 if Path(path).parent == self.destination else mode)
        with patch.object(volume, 'put', side_effect=restricted):
            with self.assertRaises(RuntimeError): self.prepare()
        self.assertFalse((self.evidence / 'PREPARATION.json').exists())
        self.assertTrue((self.evidence / 'FAILED.json').exists())
        self.assertEqual((self.destination / 'PTEditorPointerV2').read_bytes(), self.native)

    def test_tracking_flag_drift_is_not_normalized_or_ignored(self):
        if not hasattr(os, 'chflags') or not hasattr(stat, 'UF_TRACKED'): self.skipTest('Darwin metadata')
        ref = self.prepare(); path = self.destination / 'PTEditorPointerV2'
        os.chflags(path, path.stat().st_flags | stat.UF_TRACKED)
        with self.assertRaises(RuntimeError): volume.verify(ref)
        self.assertTrue(path.stat().st_flags & stat.UF_TRACKED)

    def test_prepared_directory_time_drift_refuses(self):
        ref = self.prepare(); st = self.destination.stat()
        os.utime(self.destination, ns=(st.st_atime_ns, st.st_mtime_ns + 1000000))
        with self.assertRaises(RuntimeError): volume.verify(ref)

    def test_manufactured_admission_or_profile_change_refuses(self):
        ref = self.prepare(); original = json.loads(volume.pinned(ref))
        for key, value in (('runtimeAdmission', True), ('appendASCII', 'changed'),
                           ('guestCandidate', 'Dev:Tests/other')):
            record = copy.deepcopy(original); record[key] = value
            path = Path(ref['path']); path.write_text(json.dumps(record))
            with self.assertRaises(RuntimeError): volume.verify(volume.read(path)[1])


if __name__ == '__main__': unittest.main()
