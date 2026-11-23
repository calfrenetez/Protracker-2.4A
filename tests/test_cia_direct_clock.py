import importlib.util
from pathlib import Path
import unittest


path = Path(__file__).resolve().parents[1] / "tools" / "replay_cia_direct_clock.py"
spec = importlib.util.spec_from_file_location("replay_cia_direct_clock", path)
replay = importlib.util.module_from_spec(spec)
spec.loader.exec_module(replay)


def triplet(count):
    high, low = divmod(count, 256)
    return f"{high:02x}{low:02x}{high:02x}"


def trace():
    report = ["CIA direct h=709379 end_h=709379 first=00000001:00000001 last=00000001:00001000 chip=0 timer=1 captured=48 bound=1"]
    for index in range(48):
        first = 65000 - index * 21
        last = first - (15 if index % 2 else 5)
        after = triplet(last) if index != 24 else "fcfdfb"
        report.append(f"CIA pair {index:02} {triplet(first)} {after} 2452")
    report.append("CIA direct observed verified=47 invalid=1 stray=0 closed=1 rc=0")
    return "\n".join(report)


class DirectClockReplay(unittest.TestCase):
    def test_decodes_stable_samples_and_retains_one_torn_pair(self):
        observed = replay.replay(trace())
        self.assertEqual(observed["stable_pairs"], 47)
        self.assertEqual(observed["torn_or_zero"], 1)
        self.assertEqual(observed["min_ticks"], 5)
        self.assertEqual(observed["max_ticks"], 15)
        self.assertEqual(observed["frame_ticks"], [14, 15])
        self.assertEqual(observed["shorter_than_minimum_frame_width"], 23)
        self.assertEqual(observed["at_least_maximum_frame_width"], 24)

    def test_refuses_wrong_horizon_rate_summary_and_marker(self):
        for old, new in [
            ("end_h=709379", "end_h=715909"),
            ("last=00000001:00001000", "last=00000001:00020000"),
            ("last=00000001:00001000", "last=00000001:00000100"),
            ("verified=47", "verified=48"),
            ("stray=0", "stray=1"),
            (" 2452", " 0000"),
        ]:
            with self.subTest(old=old, new=new), self.assertRaises(ValueError):
                replay.replay(trace().replace(old, new, 1))

    def test_refuses_reordered_or_wrapped_samples(self):
        report = trace().splitlines()
        report[3] = "CIA pair 02 ffffff fefefe 2452"
        with self.assertRaises(ValueError):
            replay.replay("\n".join(report))
        with self.assertRaises(ValueError):
            replay.replay(trace().replace("CIA pair 16", "CIA pair 17"))
