import importlib.util
from pathlib import Path
import unittest


path = Path(__file__).resolve().parents[1] / "tools" / "replay_cia_local_activation.py"
spec = importlib.util.spec_from_file_location("replay_cia_local_activation", path)
replay = importlib.util.module_from_spec(spec)
spec.loader.exec_module(replay)


def triplet(count):
    high, low = divmod(count, 256)
    return f"{high:02x}{low:02x}{high:02x}"


def trace():
    anchor = 65000
    hz = 709379
    result = [f"CIA local hz={hz} end_hz={hz} first=00000001:00000000 last=00000001:00008000 "
              f"chip=0 timer=0 anchor={triplet(anchor)} captured=14 complete=1"]
    for i, frame in enumerate(replay.FRAMES):
        first = (frame * hz + 47999) // 48000
        end = ((frame + 1) * hz + 47999) // 48000
        begin_count, end_count = anchor - first, anchor - end
        if i == 12:
            attempts, status, pre, post, mark = 64, 0, triplet(35000), "000000", "0000"
        elif i == 13:
            attempts, status, pre, post, mark = 1, 2, triplet(34000), "000000", "0000"
        else:
            attempts, status, pre, mark = 8, 1, triplet(begin_count - 1), "4752"
            post = triplet(end_count if i == 1 else begin_count - 8)
        result.append(f"CIA frame {i:02d} frame={frame} bounds={first}/{end} count={begin_count}/{end_count} "
                      f"attempts={attempts} status={status} pre={pre} post={post} mark={mark}")
    result.append("CIA local finished on=11 outside=1 missed=0 stray=0 closed=1 rc=0")
    return "\n".join(result)


class CiaLocalActivationReplay(unittest.TestCase):
    def test_keeps_a_real_write_at_the_excluded_tick(self):
        result = replay.replay(trace())
        self.assertEqual((result["inside"], result["issued_outside_or_torn"]), (11, 1))
        self.assertEqual(result["rows"][1]["bracket"][1], result["rows"][1]["excluded"])
        self.assertEqual([r["marker"] for r in result["rows"][12:]], [0, 0])

    def test_rejects_invented_window_write_or_receipt(self):
        for old, new in [
            ("frame=512", "frame=513"),
            ("status=0", "status=1"),
            ("attempts=64", "attempts=63"),
            ("mark=4752", "mark=0000"),
            ("on=11 outside=1", "on=12 outside=0"),
            ("last=00000001:00008000", "last=00000001:00000100"),
        ]:
            with self.subTest(old=old, new=new), self.assertRaises(ValueError):
                replay.replay(trace().replace(old, new, 1))

    def test_torn_after_keeps_real_write_but_cannot_confirm_timing(self):
        rows = trace().splitlines()
        start = rows[2].index(" post=") + len(" post=")
        rows[2] = rows[2][:start] + "020303" + rows[2][start + 6:]
        result = replay.replay("\n".join(rows))
        self.assertEqual(result["rows"][1]["verdict"], "issued_outside_or_torn")
        self.assertEqual(result["rows"][1]["marker"], 0x4752)
        self.assertIsNone(result["rows"][1]["bracket"])
