from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
RAM_PORT_SOURCES = [
    'tests/native_ram_port_test.c',
    'tests/native_ram_port.c',
    'src/core/readers_activation.c',
    'src/core/scheduled_readers.c',
    'src/core/elapsed_clock.c',
]


class NativeRamPortSoftware(unittest.TestCase):
    def test_actual_public_domains_and_original_windows(self):
        with tempfile.TemporaryDirectory() as tmp:
            executable = Path(tmp) / 'test'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-Isrc/core',
                *RAM_PORT_SOURCES, '-o', str(executable),
            ], cwd=ROOT, check=True)
            subprocess.run([str(executable)], check=True)
