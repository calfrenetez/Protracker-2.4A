import hashlib
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from editor_capture_volume import profile_append

class CaptureVolumeTests(unittest.TestCase):
    def setUp(self):
        # Synthetic bytes and an explicitly mocked expected digest keep these
        # HOST tests independent of live shared configuration or target state.
        prefix = (b'# HOST SYNTHETIC PROFILE\n'
                  b'filesystem2=rw,DH2:Dev:/host-fixture/Dev,-128\n'
                  b'uaehf1=dir,rw,DH2:Dev:/host-fixture/Dev,-128\n')
        self.original = prefix + b'#' * (4828 - len(prefix) - 1) + b'\n'
        mocked = patch('editor_capture_volume.ORIGINAL_PROFILE_SHA256',
                       hashlib.sha256(self.original).hexdigest())
        mocked.start()
        self.addCleanup(mocked.stop)

    def test_additive_suffix_retains_existing_mounts(self):
        directory = '/private/tmp/pt24g-editor-capture-volume-host-test'
        suffix = profile_append(self.original, directory)
        self.assertEqual(suffix, ('absolute_mouse=mousehack\n'
            'filesystem2=rw,DH3:PTCapture:' + directory + ',-128\n'
            'uaehf2=dir,rw,DH3:PTCapture:' + directory + ',-128\n').encode())
        proposed = self.original + suffix
        self.assertTrue(proposed.startswith(self.original))
        self.assertEqual(proposed.count(b'filesystem2=rw,DH2:Dev:'), 1)
        self.assertEqual(proposed.count(b'uaehf1=dir,rw,DH2:Dev:'), 1)
        self.assertNotEqual(hashlib.sha256(proposed).digest(), hashlib.sha256(self.original).digest())

    def test_rejects_changed_or_already_patched_profile(self):
        for body in (self.original + b'absolute_mouse=mousehack\n', b'',
                     b'X' + self.original[1:], self.original.decode('ascii')):
            with self.subTest(body_type=type(body).__name__):
                with self.assertRaises(ValueError):
                    profile_append(body, '/private/tmp/pt24g-editor-capture-volume-host-test')

    def test_rejects_nonlocal_ambiguous_or_injected_mounts(self):
        for path in ('/tmp/pt24g-editor-capture-volume-a', '/private/tmp/other',
                     '/private/tmp/pt24g-editor-capture-volume-',
                     '/private/tmp/pt24g-editor-capture-volume-../other',
                     '/private/tmp/pt24g-editor-capture-volume-a,b',
                     '/private/tmp/pt24g-editor-capture-volume-a\nuaehf0=bad',
                     '/private/tmp/pt24g-editor-capture-volume-a b',
                     '/private/tmp/pt24g-editor-capture-volume-' + 'a' * 49, None):
            with self.subTest(path=path):
                with self.assertRaises(ValueError):
                    profile_append(self.original, path)


if __name__ == '__main__':
    unittest.main()
