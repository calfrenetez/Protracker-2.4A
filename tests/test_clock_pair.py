from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/clock_pair_test.c', 'tests/clock_pair.c']
class ClockPair(unittest.TestCase):
    def test_full_width_saved_observations(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'clock-pair'
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', *SOURCES, '-o', str(binary)], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)
