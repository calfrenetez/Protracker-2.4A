from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class PaulaVoices(unittest.TestCase):
    def test_routed_readers_and_confirmed_release(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core','tests/paula_voices_test.c',
                            'src/editor/paula_voices.c','src/editor/sampler_paula.c',
                            'src/core/sample_cache.c','src/core/playback_pcm.c',
                            *SOURCES[1:],'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
