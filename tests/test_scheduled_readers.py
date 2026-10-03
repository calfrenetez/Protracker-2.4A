"""Portable split-domain persistent-reader and recyclable-command contract."""
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# Production scheduled_readers.c is included exactly once by this white-box
# fixture for bounded ticket/serial rollover checks; never compile it twice.
SOURCES = ['tests/scheduled_readers_test.c', 'src/core/elapsed_clock.c']

class ScheduledReadersTest(unittest.TestCase):
    def test_contract(self):
        with tempfile.TemporaryDirectory() as d:
            binary = Path(d) / 'scheduled-readers-test'
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                            '-g', *SOURCES, '-o', str(binary)], cwd=ROOT, check=True)
            subprocess.run([str(binary)], cwd=ROOT, check=True)

if __name__ == '__main__':
    unittest.main()
