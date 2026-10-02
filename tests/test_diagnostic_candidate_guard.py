import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('diagnostic_guard', ROOT / 'tools/diagnostic_candidate_guard.py')
guard = importlib.util.module_from_spec(spec)
spec.loader.exec_module(guard)


class DiagnosticCandidateGuard(unittest.TestCase):
    def test_stale_working_or_committed_inputs_cannot_qualify_exact_bytes(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            def git(*args):
                return subprocess.run(['git', *args], cwd=root, check=True,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            git('init')
            source = root / 'source.c'; source.write_bytes(b'original source')
            header = root / 'vendor/amigus-sdk/amigus.h'; header.parent.mkdir(parents=True)
            header.write_bytes(b'pinned SDK header')
            lock = root / 'amigus-sdk.lock.json'
            lock.write_text(json.dumps({'files': {'amigus.h': guard.digest(header)}}))
            git('add', '.')
            git('-c', 'user.name=Fixture', '-c', 'user.email=fixture@example.invalid', 'commit', '-m', 'fixture')
            binary = root / 'AmiGUSTest'; binary.write_bytes(b'exact native bytes')
            manifest = root / 'build.json'
            original = dict(binary_sha256=guard.digest(binary), binary_bytes=binary.stat().st_size,
                inputs={'source.c': guard.digest(source)}, sdk_lock_sha256=guard.digest(lock))
            manifest.write_text(json.dumps(original))
            with patch.object(guard, 'ARTIFACTS', [('AmiGUSTest', 'build.json')]):
                self.assertEqual(guard.verify_diagnostic_candidates(root), {'AmiGUSTest': guard.digest(binary)})
                source.write_bytes(b'uncommitted changed source')
                with self.assertRaisesRegex(RuntimeError, 'source differs'):
                    guard.verify_diagnostic_candidates(root)
                # A locally edited manifest cannot promote those uncommitted inputs.
                altered = dict(original, inputs={'source.c': guard.digest(source)})
                manifest.write_text(json.dumps(altered))
                with self.assertRaisesRegex(RuntimeError, 'committed build'):
                    guard.verify_diagnostic_candidates(root)
                source.write_bytes(b'original source'); manifest.write_text(json.dumps(original))
                binary.write_bytes(b'unqualified native bytes')
                with self.assertRaisesRegex(RuntimeError, 'binary differs'):
                    guard.verify_diagnostic_candidates(root)
                binary.write_bytes(b'exact native bytes'); header.write_bytes(b'changed SDK')
                with self.assertRaisesRegex(RuntimeError, 'SDK header changed'):
                    guard.verify_diagnostic_candidates(root)
                header.write_bytes(b'pinned SDK header')
                for altered in (dict(original, inputs={}), dict(original, sdk_lock_sha256=None),
                                dict(original, binary_bytes=1)):
                    manifest.write_text(json.dumps(altered))
                    with self.assertRaises(RuntimeError):guard.verify_diagnostic_candidates(root)
                manifest.write_text(json.dumps(original)); lock.write_text('{"files":{}}')
                with self.assertRaisesRegex(RuntimeError, 'SDK lock differs'):
                    guard.verify_diagnostic_candidates(root)


if __name__ == '__main__':
    unittest.main()
