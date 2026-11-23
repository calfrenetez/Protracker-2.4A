from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from replay_midi_wire import replay


def triplet(count):
    return f"{count:04x}{count >> 8:02x}"


def receipt():
    anchor, hz = 65520, 709379
    lines = [f"MIDI wire clock hz={hz} timer=0/0 alarm=0/1 anchor={triplet(anchor)} epoch=8192 period=113"]
    for index, (frame, tx, rx) in enumerate(((0, "195/13c/15a", "953c5a"), (1920, "185/13c/100", "853c00"),
                                            (3000, "195/13c/15a", "000000"))):
        start, end = (8192 + (edge * hz + 47999) // 48000 for edge in (frame, frame + 1))
        first, excluded = anchor - start, anchor - end
        expired = index == 2
        threshold = excluded - 128 if expired else first + 128
        pre, post = (excluded - 115, 0) if expired else (first - 1, first - 7)
        lines.append(f"MIDI wire {index} frame={frame} window={start}/{end} bound={first}/{excluded} "
                     f"arm={triplet(threshold + 1000)} delay=1000 calls={index+1} status={2 if expired else 1} "
                     f"attempts=2 pre={triplet(pre)} post={triplet(post)} mark={'0000' if expired else '4752'} "
                     f"tx={tx} rx={rx} received={0 if expired else 3} idle=1")
    return "\n".join(lines + ["MIDI wire finished callbacks=3 stray=0/0 closed=1 rc=0"])


class MidiWireTest(unittest.TestCase):
    def test_exact_single_note_encoder(self):
        with tempfile.TemporaryDirectory() as path:
            executable = Path(path) / "midi-wire"
            subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-fsanitize=address,undefined",
                            "tests/routed_midi_wire_test.c", "src/core/routed_midi_wire.c", "-o", str(executable)],
                           cwd=ROOT, check=True)
            subprocess.run([str(executable)], check=True)

    def test_timing_and_loopback_readback(self):
        report = receipt()
        self.assertEqual(replay(report)["notes"][0]["first_write_ticks"], [8193, 8199])
        for old, new in (("received=3", "received=2"), ("853c00", "853c01"), ("frame=3000", "frame=3001"),
                         ("period=113", "period=112"), ("rc=0", "rc=20"), ("received=0", "received=1")):
            with self.subTest(changed=new), self.assertRaises(ValueError):
                replay(report.replace(old, new, 1))
