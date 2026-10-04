from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/sample_range_test.c', 'src/editor/sample_range.c']
class SampleRange(unittest.TestCase):
    def test_half_open_ranges_and_source_preservation(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'sample-range-test')
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', *SOURCES, '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True)
