from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from replay_routed_paula_alarm import replay
from replay_tracker_paula_alarm import replay as replay_tracker


def triplet(count):
    return f"{count:04x}{count >> 8:02x}"


def fixture(frames=(512, 1280), epoch=0):
    anchor = 65523
    rate = 709379
    lines = []
    arms = []
    windows = []
    device = [
        (2, "initial-owned-state", 0x200, 0, 0x080, 0, 0, 0),
        (2, "after-request-wait", 0x200, 0, 0, 0, 0, 0),
        (3, "after-registers", 0x20F, 0, 0, 1, 0x96, 0x800F),
        (3, "after-start-fetches", 0x20F, 0x780, 0, 0, 0, 0),
        (6, "after-registers", 0x200, 0x780, 0, 1, 0x96, 0x000F),
        (6, "after-request-wait", 0x200, 0, 0, 0, 0, 0),
        (7, "interrupt-mask-restored", 0x200, 0, 0x080, 0, 0, 0),
    ]
    for i, (stage, point, dma, request, enabled, writes, reg, value) in enumerate(device):
        lines.append(
            f"Paula trace {i:02d} stage={stage} point={point} dma={dma:04x} req={request:04x} "
            f"ena={enabled:04x} adk=0000 owned=1 writes={writes} reg={reg:04x} value={value:04x}"
        )
    for i, (stage, frame, word, before, after) in enumerate(((3, frames[0], 0x800F, 0x200, 0x20F),
                                                          (6, frames[1], 0x000F, 0x20F, 0x200))):
        first, end = (anchor - epoch - (edge * rate + 47999) // 48000 for edge in (frame, frame + 1))
        probe = first + 500
        arms.append(f"Paula alarm {i} sample={triplet(probe)} lead={first+128} delay=372 calls={i+1}")
        windows.append(
            f"Paula window {i} stage={stage} hz={rate} chip=0 timer=0 anchor={triplet(anchor)} count={first}/{end} "
            f"attempts=9 status=1 rawbefore={triplet(first-1)} rawafter={triplet(first-7)} mark=4752 "
            f"command={word:04x} dma={before:04x}/{after:04x} in_frame=1"
        )
    return "\n".join(lines + arms + ["Paula alarm closed=1 chip=0 timer=1 calls=2 stray=0"] +
                     windows + ["Paula windows closed stray=0 rows=2", "Paula trace finished stage=6 rc=0 owned-closed=1"])


class PaulaAlarmReplay(unittest.TestCase):
    def test_shared_epoch_and_original_device_custody(self):
        evidence = replay(fixture())
        self.assertEqual(evidence["clock_anchor_count"], 65523)
        self.assertEqual(evidence["alarm"]["callbacks"], 2)
        self.assertEqual([row["observed_ticks"][1] - row["observed_ticks"][0] for row in evidence["writes"]], [6, 6])

        for original, changed in (
            ("timer=1 calls=2", "timer=0 calls=2"),
            ("delay=372 calls=2", "delay=372 calls=1"),
            ("delay=372 calls=2", "delay=373 calls=2"),
            (f"anchor={triplet(65523)}", f"anchor={triplet(65522)}"),
            ("point=after-start-fetches dma=020f req=0780", "point=after-start-fetches dma=020f req=0000"),
            ("point=interrupt-mask-restored dma=0200 req=0000 ena=0080",
             "point=interrupt-mask-restored dma=0200 req=0000 ena=0000"),
            ("owned=1", "owned=0"),
        ):
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                replay(fixture().replace(original, changed, 1))

    def test_tracker_epoch_and_f00_cannot_be_shifted(self):
        forecast = "Paula tracker epoch=8192 start=0 stop=1920 paula=4 amigus=1 midi=1 f00=1"
        text = forecast + "\n" + fixture((0, 1920), 8192)
        evidence = replay_tracker(text)
        self.assertEqual(evidence["writes"][0]["frame_ticks"], [8192, 8207])
        self.assertEqual(evidence["writes"][1]["frame_ticks"], [36568, 36582])
        with self.assertRaises(ValueError):
            replay_tracker(text.replace("stop=1920", "stop=1919", 1))
        with self.assertRaises(ValueError):
            replay_tracker(forecast + "\n" + fixture((0, 1920), 8191))
