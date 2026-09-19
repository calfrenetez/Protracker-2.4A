from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class Channels(unittest.TestCase):
    def test_navigation_routing_and_history(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'channels-test')
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-g', '-Isrc/core',
                            'tests/channels_test.c', 'src/core/channels.c', '-o', binary],
                           cwd=ROOT, check=True)
            subprocess.run([binary], check=True, capture_output=True)
