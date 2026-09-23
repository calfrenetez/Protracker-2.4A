from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class Scope(unittest.TestCase):
    def test_phase_loop_retrigger_pitch_and_bounds(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'scope-test')
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core',
                            'tests/scope_test.c', '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True)
