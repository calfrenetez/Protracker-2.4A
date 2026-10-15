from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/cia_prepared_aperture_test.c', 'tests/cia_prepared_aperture.c',
           'tests/cia_aperture_model.c', 'src/core/elapsed_clock.c']
class PreparedAperture(unittest.TestCase):
    def test_preparation_and_full_width_replay(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'cia-prepared-aperture'
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', *SOURCES, '-o', str(binary)], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)
