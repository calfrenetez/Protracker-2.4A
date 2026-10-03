"""Portable scheduled activation lineage and strict-retirement contract."""
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/scheduled_lineage_test.c', 'src/core/scheduled_lineage.c',
           'src/core/elapsed_clock.c']

class ScheduledLineageTest(unittest.TestCase):
    def test_contract(self):
        with tempfile.TemporaryDirectory() as d:
            binary = Path(d) / 'scheduled-lineage-test'
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                            '-g', *SOURCES, '-o', str(binary)], cwd=ROOT, check=True)
            subprocess.run([str(binary)], cwd=ROOT, check=True)

if __name__ == '__main__':
    unittest.main()
