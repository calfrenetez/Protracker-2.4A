import importlib.util
from pathlib import Path
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from compare_hunks import compare, parse
from make_fixture import make_mod
from prepare_asm import prepare


def hunk(payload=b'\x4e\x75\x00\x00', relocation=None):
    words = [1011, 0, 1, 0, 0, 1, 1001, 1]
    data = struct.pack('>8I', *words) + payload
    if relocation is not None:
        data += struct.pack('>5I', 1004, 1, 0, relocation, 0)
    return data + struct.pack('>I', 1010)


class BaselineTools(unittest.TestCase):
    def test_preparation_preserves_other_operands_and_comments(self):
        source = (b'label CMP.W #42,D0 ; note\n\tADD.L #4,A0\n'
                  b'\tCMP.W D1,D0\n; CMP.W #42,D0\n'
                  b'\tdc.b "CMP.W #42,D0",0\n')
        result, counts = prepare(source)
        self.assertEqual(result, source.replace(b'label CMP.W', b'label CMPI.W'))
        self.assertEqual(counts, {'CMP': 1})

    def test_preparation_is_idempotent(self):
        once, _ = prepare(b'\tAND.L #$ffff,D7\n')
        self.assertEqual(prepare(once), (once, {}))

    def test_same_layout_is_not_enough(self):
        with self.assertRaises(ValueError):
            compare(hunk(), hunk(b'\x4e\x71\x00\x00'))

    def test_truncated_and_trailing_hunks_rejected(self):
        for data in (hunk()[:-1], hunk() + b'junk'):
            with self.assertRaises(ValueError):
                parse(data)

    def test_out_of_bounds_relocation_rejected(self):
        with self.assertRaises(ValueError):
            parse(hunk(relocation=2))

    def test_relocation_difference_rejected(self):
        with self.assertRaises(ValueError):
            compare(hunk(), hunk(relocation=0))

    def test_fixture_is_bounded_classic_mod(self):
        data = make_mod()
        self.assertEqual(data[1080:1084], b'M.K.')
        self.assertEqual(data[950], 1)
        self.assertEqual(len(data), 1084 + 1024 + 64)
        sample_words = struct.unpack_from('>H', data, 42)[0]
        loop_start, loop_words = struct.unpack_from('>HH', data, 46)
        self.assertLessEqual(loop_start + loop_words, sample_words)
        self.assertEqual(data, make_mod())


if __name__ == '__main__':
    unittest.main()
