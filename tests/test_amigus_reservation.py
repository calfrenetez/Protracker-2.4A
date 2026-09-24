from pathlib import Path
import subprocess, tempfile, unittest
ROOT = Path(__file__).resolve().parents[1]
class AmiGusReservation(unittest.TestCase):
    def test_library_lifetime(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / 'test'
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra',
                            '-Werror', '-fsanitize=address,undefined', '-Isrc/core',
                            'tests/amigus_reservation_test.c',
                            'src/core/amigus_reservation.c', '-o', str(exe)],
                           cwd=ROOT, check=True)
            subprocess.run([str(exe)], check=True)
