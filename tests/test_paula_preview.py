from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class PaulaPreview(unittest.TestCase):
    def test_filtered_preview(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'paula-preview-test')
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core',
                            'tests/paula_preview_test.c', 'src/core/pcm_filtered.c',
                            'src/core/sample_cache.c', 'src/core/pcm.c', '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True)
