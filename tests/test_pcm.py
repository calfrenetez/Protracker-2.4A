from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class Pcm(unittest.TestCase):
    def test_precision_operations_and_wav(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'pcm-test')
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-g', '-Isrc/core',
                            'tests/pcm_test.c', 'src/core/pcm.c', 'src/core/wav.c', '-o', binary],
                           cwd=ROOT, check=True)
            subprocess.run([binary], check=True, capture_output=True)
