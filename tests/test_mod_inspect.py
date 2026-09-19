from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ModInspect(unittest.TestCase):
    def test_bounds_and_truncation(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'mod-test')
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-g', '-Isrc/core',
                            'tests/mod_inspect_test.c', 'src/core/mod_inspect.c',
                            '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True, capture_output=True)
