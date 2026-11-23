from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from replay_cia_tail import replay


LOG = (
    "CIA tail hz=709379 clock=0/0 alarm=0/1 original=fff0ff after=fe00fe after_tick=66032 reloads=1\n"
    "CIA tail plan frame=65507/65522 aperture=65507/65519 count=13/1 too_short=65515/65530 refused=1\n"
    "CIA tail issue generation=0 status=1 arm=200020 lead=141 delay=8051 attempts=9 "
    "pre=000c00 post=000600 mark=4752 ticks=65508/65514\n"
    "CIA tail finished callbacks=1 stray=0 closed=1 rc=0\n"
)


class CiaTailReplay(unittest.TestCase):
    def test_original_crossing_and_strict_same_generation_issue(self):
        verified = replay(LOG)
        self.assertEqual(verified["reload_tick"], 65521)
        self.assertEqual(verified["observed_ticks"], [65508, 65514])
        for old, new in (("frame=65507/65522", "frame=65507/65521"),
                         ("aperture=65507/65519", "aperture=65507/65522"),
                         ("refused=1", "refused=0"),
                         ("generation=0", "generation=1"),
                         ("post=000600", "post=fffffe"),
                         ("closed=1 rc=0", "closed=1 rc=20")):
            with self.subTest(changed=new), self.assertRaises(ValueError):
                replay(LOG.replace(old, new, 1))
