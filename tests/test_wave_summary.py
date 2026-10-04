from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/wave_summary_test.c', 'src/editor/wave_summary.c']
class WaveSummary(unittest.TestCase):
    def test_bounded_private_overview(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'wave-summary-test')
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', *SOURCES, '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True)
