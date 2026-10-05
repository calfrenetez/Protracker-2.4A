from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
RETIREMENT_SOURCES = [
    'tests/readers_activation_retirement_test.c',
    'tests/native_ram_port.c',
    'src/core/readers_activation.c',
    'src/core/scheduled_readers.c',
    'src/core/elapsed_clock.c',
]


class ReadersActivationRetirement(unittest.TestCase):
    def test_reserved_reader_retirement_is_independent_from_command(self):
        with tempfile.TemporaryDirectory() as tmp:
            executable = Path(tmp) / 'test'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-Isrc/core',
                *RETIREMENT_SOURCES, '-o', str(executable),
            ], cwd=ROOT, check=True)
            subprocess.run([str(executable)], check=True)
