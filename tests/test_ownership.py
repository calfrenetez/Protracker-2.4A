from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class Ownership(unittest.TestCase):
    def test_driver_failure_and_cleanup(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'ownership-test')
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-g',
                            '-Isrc/diagnostic', 'tests/ownership_test.c',
                            'src/diagnostic/ownership.c', '-o', binary],
                           cwd=ROOT, check=True)
            result = subprocess.run([binary], check=True, text=True, capture_output=True)
            self.assertIn('18 scenarios passed', result.stdout)
