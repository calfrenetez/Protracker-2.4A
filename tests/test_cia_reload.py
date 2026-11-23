from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from replay_cia_reload import MAX_RELOADS, MIN_RELOADS, PERIOD, replay


def fixture(spread=20, calls=MIN_RELOADS, wakes=1000):
    delta = calls * PERIOD + 30
    return "\n".join((
        f"CIA reload probe 0 before=00000003:000003e8 after=00000003:{1000+spread:08x} calls=0/0 count=fff0ff",
        f"CIA reload probe 1 before=00000003:{1000+delta:08x} after=00000003:{1000+delta+spread:08x} "
        f"calls={calls}/{calls} count=ffd2ff",
        f"CIA reload finished hz=709379 chip=0 timer=0 wakes={wakes} calls={calls} "
        f"bounds={delta-spread}/{delta+spread} ticks={delta} closed=1 rc=0",
    ))


class CiaReloadReplay(unittest.TestCase):
    def test_original_elapsed_time_and_neighbors(self):
        observed = replay(fixture())
        self.assertEqual(observed["selected_period_ticks"], 65536)
        delta = MIN_RELOADS * PERIOD + 30
        self.assertEqual(observed["reconstructed_elapsed_ticks"], delta)
        self.assertEqual(observed["wake_observations"], 1000)
        self.assertEqual(replay(fixture(calls=MIN_RELOADS + 1))["reloads"], MIN_RELOADS + 1)
        with self.assertRaises(ValueError):
            replay(fixture(spread=MIN_RELOADS))
        for calls, wakes in ((MIN_RELOADS - 1, 1000), (MAX_RELOADS + 1, 1000), (MIN_RELOADS, MIN_RELOADS + 1)):
            with self.subTest(calls=calls, wakes=wakes), self.assertRaises(ValueError):
                replay(fixture(calls=calls, wakes=wakes))
        for original, changed in ((f"calls={MIN_RELOADS}/{MIN_RELOADS}", f"calls={MIN_RELOADS}/{MIN_RELOADS+1}"),
                                  ("count=ffd2ff", "count=ffd2fe"),
                                  (f"ticks={delta}", f"ticks={delta-1}"), ("rc=0", "rc=20")):
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                replay(fixture().replace(original, changed, 1))
