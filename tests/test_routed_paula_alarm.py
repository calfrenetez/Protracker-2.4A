from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from replay_routed_paula_alarm import replay
from replay_tracker_paula_alarm import replay as replay_tracker
from replay_mixed_frame_aperture import observe as observe_mixed, replay as replay_mixed
from replay_mixed_frame_wait import replay as replay_wait
from replay_mixed_frame_reload import replay as replay_reload


def triplet(count):
    return f"{count:04x}{count >> 8:02x}"


def fixture(frames=(512, 1280), epoch=0, anchors=((0, 65523), (0, 65523))):
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
        anchor_tick, anchor = anchors[i]
        first, end = (anchor_tick + anchor - epoch - (edge * rate + 47999) // 48000
                      for edge in (frame, frame + 1))
        probe = first + 500
        arms.append(f"Paula alarm {i} sample={triplet(probe)} lead={first+128} delay=372 calls={i+1}")
        windows.append(
            f"Paula window {i} stage={stage} hz={rate} chip=0 timer=0 anchor={triplet(anchor)} count={first}/{end} "
            f"attempts=9 status=1 rawbefore={triplet(first-1)} rawafter={triplet(first-7)} mark=4752 "
            f"command={word:04x} dma={before:04x}/{after:04x} in_frame=1"
        )
    return "\n".join(lines + arms + ["Paula alarm closed=1 chip=0 timer=1 calls=2 stray=0"] +
                     windows + ["Paula windows closed stray=0 rows=2", "Paula trace finished stage=6 rc=0 owned-closed=1"])


def mixed_fixture(stop=1920, anchors=((0, 65523), (0, 65523))):
    return (
        f"Paula tracker epoch=8192 start=0 stop={stop} paula=4 amigus=1 midi=1 f00=1\n"
        "Mixed IRQ RAM 0 bank=0000/0000 control=8000/8000 midi=01953c5a/01953c5a\n"
        "Mixed IRQ RAM 1 bank=0000/0000 control=0000/0000 midi=01853c00/01853c00\n"
        + fixture((0, stop), 8192, anchors)
    )


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

    def test_mixed_ram_readbacks_and_irreversible_late_issue(self):
        text = mixed_fixture()
        result = replay_mixed(text)
        self.assertEqual([r["verdict"] for r in result["mixed_ram_observations"]], ["issued_in_frame"] * 2)
        after = "mark=4752 command=800f dma=0200/020f"
        late = text.replace(f"rawafter={triplet(65523 - 8199)} {after} in_frame=1",
                            f"rawafter={triplet(65523 - 8207)} {after} in_frame=0", 1)
        self.assertEqual(observe_mixed(late)[0]["verdict"], "issued_outside")
        with self.assertRaises(ValueError):
            replay_mixed(late)
        with self.assertRaises(ValueError):
            replay_mixed(text.replace("01953c5a/01953c5a", "01953c5a/00953c5a", 1))

    def test_original_alarm_is_the_only_wait_wake(self):
        rows = []
        for index, first in enumerate((8192, 36568)):
            entering = triplet(65523 - first + 300)
            waking = triplet(65523 - first - 80)
            rows.append(f"Paula wait {index} before={entering} after={waking} alarm=1")
        text = "\n".join(rows) + "\n" + mixed_fixture()
        result = replay_wait(text)
        self.assertEqual(result["wait_observations"][0]["before_and_after_wait"], [7892, 8272])
        with self.assertRaises(ValueError):
            replay_wait(text.replace("alarm=1", "alarm=0", 1))
        with self.assertRaises(ValueError):
            replay_wait(text.replace(f"before={triplet(65523 - 8192 + 300)}", f"before={triplet(65523 - 8193)}", 1))

    def test_mixed_wait_crosses_one_original_reload(self):
        start_count, later_count = 65523, 65000
        later_tick = 65536 + start_count - later_count
        stop_frame = 5760
        stop_tick = 8192 + (stop_frame * 709379 + 47999) // 48000
        rows = ["Paula reload clock callbacks=1"]
        for index, (tick, count, target) in enumerate(((0, start_count, 8192), (later_tick, later_count, stop_tick))):
            rows.extend((
                f"Paula reload {index} tick={tick} planned={index} issued={index}",
                f"Paula wait {index} before={triplet(tick+count-target+300)} "
                f"after={triplet(tick+count-target-80)} alarm=1",
            ))
        text = "\n".join(rows) + "\n" + mixed_fixture(stop_frame, ((0, start_count), (later_tick, later_count)))
        result = replay_reload(text)
        self.assertEqual(result["clock_reload"], {"origin_sample_tick": later_tick, "original_callbacks": 1})
        self.assertEqual(result["wait_observations"][1]["before_and_after_wait"], [stop_tick - 300, stop_tick + 80])
        self.assertEqual(result["writes"][1]["observed_ticks"], [stop_tick + 1, stop_tick + 7])
        for original, changed in (("planned=1 issued=1", "planned=1 issued=0"),
                                  (f"tick={later_tick}", f"tick={later_tick+1}"),
                                  ("clock callbacks=1", "clock callbacks=2"),
                                  ("stop=5760", "stop=5759")):
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                replay_reload(text.replace(original, changed, 1))
