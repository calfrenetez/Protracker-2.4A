from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class Slices(unittest.TestCase):
    def test_transients_markers_and_crossfade(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'slices-test')
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core', 'tests/slices_test.c',
                            'src/core/slices.c', 'src/core/pcm.c', '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True)
