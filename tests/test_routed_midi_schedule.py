from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class RoutedMidiSchedule(unittest.TestCase):
    def test_timed_batch_lifetime_and_exact_receipt(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / "routed-midi"
            subprocess.run([
                "cc", "-std=c99", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                "-fsanitize=address,undefined", "-Isrc/core",
                "tests/routed_midi_schedule_test.c", "src/core/routed_midi.c",
                "src/core/midi.c", "src/core/channels.c", "src/core/elapsed_clock.c",
                "-o", str(binary),
            ], cwd=ROOT, check=True, timeout=60)
            subprocess.run([str(binary)], check=True, timeout=15)
