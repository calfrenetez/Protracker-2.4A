from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class Pattern(unittest.TestCase):
    def test_block_edits_and_history(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'pattern-test')
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core', 'tests/pattern_test.c',
                            'src/core/pattern.c', 'src/core/project.c', 'src/core/channels.c',
                            'src/core/pcm.c', '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True)
