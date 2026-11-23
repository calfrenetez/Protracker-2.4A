from pathlib import Path
import sys
import unittest

from test_cia_local_activation import trace, triplet

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from replay_cia_alarm_frames import replay
from replay_cia_local_activation import replay as replay_task


def trace_with_alarm(refusal=False):
    lines = trace().splitlines()
    task = replay_task(trace())
    anchor = task["clock_anchor_count"]
    calls = 0
    arms = []
    for i, row in enumerate(task["rows"]):
        if i < 12:
            threshold = anchor - row["first"] + 128
            probe = threshold + 128
        else:
            probe, d = (36500, 1024) if i == 12 else (34500, 128)
            threshold = probe - d
        if refusal and i == 0:
            probe = threshold - 1
            lines[1] = (lines[1].split(" attempts=")[0] +
                        f" attempts=0 status=3 pre={triplet(probe)} post=000000 mark=0000")
            lines[15] = lines[15].replace("on=11 outside=1 missed=0", "on=10 outside=1 missed=1")
            delta, wake, phase = 0, 0, 2
        else:
            delta, wake, phase = probe - threshold, 1, 5
            calls += 1
        arms.append(f"CIA alarm {i:02d} probe={probe} raw={triplet(probe)} wake_at={threshold} "
                    f"delta={delta} wake={wake} phase={phase} calls={calls}")
    lines.extend(arms)
    lines.append(f"CIA alarm closed chip=1 timer=1 calls={calls} stray=0 attempted=14")
    return "\n".join(lines)


class CiaAlarmFrames(unittest.TestCase):
    def test_original_timer_identity_and_callback_count(self):
        observed = replay(trace_with_alarm())
        self.assertEqual(observed["alarm_source"]["callbacks"], 14)
        self.assertEqual(observed["inside"], 11)
        for early, replacement in [
            ("chip=1 timer=1 calls=14", "chip=0 timer=0 calls=14"),
            ("calls=14 stray=0", "calls=13 stray=0"),
            ("delta=128 wake=1", "delta=129 wake=1"),
        ]:
            with self.subTest(early=early), self.assertRaises(ValueError):
                replay(trace_with_alarm().replace(early, replacement, 1))

    def test_pre_arm_refusal_keeps_source_and_makes_no_write(self):
        text = trace_with_alarm(refusal=True)
        observed = replay(text)
        self.assertEqual(observed["alarm_source"]["callbacks"], 13)
        self.assertEqual(observed["missed_before_write"], 1)
        self.assertEqual(observed["arming"][0]["verdict"], "wake_deadline_too_close")
        self.assertEqual(observed["rows"][0]["marker"], 0)
        with self.assertRaises(ValueError):
            replay_task("\n".join(text.splitlines()[:16]))
        with self.assertRaises(ValueError):
            replay(text.replace("phase=2 calls=0", "phase=2 calls=1", 1))
