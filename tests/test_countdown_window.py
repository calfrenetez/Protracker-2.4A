from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/countdown_window_test.c', 'src/core/countdown_window.c', 'src/core/elapsed_clock.c']


class CountdownWindow(unittest.TestCase):
    def test_original_half_open_bounds(self):
        with tempfile.TemporaryDirectory() as temporary:
            executable = Path(temporary) / "countdown-window"
            subprocess.run([
                "cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-Isrc/core",
                "-fsanitize=address,undefined", *SOURCES, "-o", executable,
            ], cwd=ROOT, check=True)
            subprocess.run([executable], check=True)
