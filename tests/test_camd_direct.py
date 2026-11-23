from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from replay_camd_direct import replay


def report():
    rows = []
    for group in range(8):
        origin = 0x100 + group * 0x8000
        rows.append(f"CAMD direct group {group} hz=709379 timer=0/0 start=00000001:{origin:08x} "
                    f"end=00000001:{origin+0x4000:08x} attempted=4 coherent=4")
        for index in range(4):
            ordinal = group * 4 + index
            count = 64000 - index * 100
            parts = [f"{sample:04x}{sample >> 8:02x}" for sample in (count, count - 4, count - 10, count - 44)]
            packed = 0x863C00 if ordinal & 1 else 0x963C64
            rows.append(f"CAMD direct row {ordinal} baseline={parts[0]}/{parts[1]} put={parts[2]}/{parts[3]} "
                        f"message={packed:06x} stamp={0xCAD10000|ordinal:08x} received=1")
    return "\n".join(rows + ["CAMD direct closed: original clock, library, node and links released rc=0"])


class CamdDirectReplay(unittest.TestCase):
    def test_separate_stable_counter_pairs_and_local_receipts(self):
        text = report()
        result = replay(text)
        self.assertEqual(len(result["groups"]), 8)
        self.assertEqual(result["baseline_ticks"], [4, 4])
        self.assertEqual(result["put_bracket_ticks"], [34, 34])
        self.assertEqual(result["put_brackets_narrower_than_maximum_frame"], 0)
        for before, after in (("cad1001f", "cad1001e"), ("coherent=4", "coherent=3"),
                              ("end=00000001:00004100", "end=00000002:00004100"),
                              ("end=00000001:00004100", "end=00000001:000041"),
                              ("start=00000001:00008100", "start=00000001:00003100")):
            with self.subTest(changed=after), self.assertRaises(ValueError):
                replay(text.replace(before, after, 1))
