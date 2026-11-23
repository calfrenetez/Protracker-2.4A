from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from replay_amigus_selected_control import replay


HEADER = ("WAVETABLE-PROBE scope=Mini-7ea663e7-selected-control writes=NO bank_select=NO "
          "bank_identity=UNKNOWN voices_stopped=NOT_QUALIFIED\n")
RESULT = (
    HEADER + "WAVETABLE SELECTED CONTROL offset=0x20 value=0x8003\n"
    "WAVETABLE RELEASE confirmed=1 retained=0 driver=0x00000000\n"
    "WAVETABLE result=PASS reason=observed reads=1 rc=0\n"
)


class SelectedAmiGUSControlReplay(unittest.TestCase):
    def test_readback_and_original_release(self):
        result = replay(RESULT, 0)
        self.assertEqual(result["control_word"], 0x8003)
        self.assertTrue(result["playback_control_bit"])
        self.assertTrue(result["original_release_confirmed"])
        self.assertIsNone(result["bank_identity"])
        self.assertFalse(replay(RESULT.replace("0x8003", "0x0003"), 0)["playback_control_bit"])

    def test_unknown_ownership_replay_and_unsolicited_offsets_refuse(self):
        for changed in (
            RESULT.replace("offset=0x20", "offset=0x1e"),
            RESULT.replace("bank_select=NO", "bank_select=YES"),
            RESULT.replace("confirmed=1 retained=0", "confirmed=0 retained=1"),
            RESULT.replace("driver=0x00000000", "driver=0x00000001"),
            RESULT + RESULT,
        ):
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                replay(changed, 0)
        with self.assertRaises(ValueError):
            replay(RESULT, 20)

    def test_missing_library_explicitly_writes_no_readback(self):
        text = HEADER + "WAVETABLE result=SKIP reason=library-unavailable rc=5\n"
        self.assertEqual(replay(text, 5),
                         {"selected_control_observed": False, "outcome": "library-unavailable"})
        with self.assertRaises(ValueError):
            replay(text, 0)
