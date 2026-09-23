from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class PlaybackPCM(unittest.TestCase):
    def test_representations_preserve_master(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'playback-pcm-test')
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core',
                            'tests/playback_pcm_test.c', 'src/core/playback_pcm.c',
                            'src/core/sample_cache.c', 'src/core/pcm.c', '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True)
