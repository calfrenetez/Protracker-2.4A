from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/cia_aperture_test.c', 'tests/cia_aperture_model.c', 'src/core/elapsed_clock.c']
class CiaAperture(unittest.TestCase):
    def test_original_window_and_commit_policy(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'cia-aperture'
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', *SOURCES, '-o', str(binary)], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)
