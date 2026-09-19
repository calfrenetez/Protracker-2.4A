from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from prepare_dev import prepare_dev


class NativePreparation(unittest.TestCase):
    def test_guard_precedes_destructive_load_and_changed_anchors_fail(self):
        raw = (ROOT / 'vendor/pt23f/PT2.3F.s').read_bytes()
        guard = (ROOT / 'src/native/mod_guard.s').read_bytes()
        input_source = (ROOT / 'src/native/input.s').read_bytes()
        dev, _ = prepare_dev(raw, guard, input_source)
        pos = dev.index(b'\nLoadModule\n')
        self.assertLess(dev.index(b'JSR PTGPreflight', pos), dev.index(b'BSR.W\tDoClearSong', pos))
        self.assertEqual(dev.count(b'JMP PTGReadFailed'), 4)
        with self.assertRaises(ValueError):
            prepare_dev(raw.replace(b'\nLoadModule\n', b'\nChangedLoadModule\n', 1), guard, input_source)
