from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
ROOT=Path(__file__).resolve().parents[1]
# The fixture includes the new adapter once for private lifecycle/capacity images.
READERS_SOURCES=['tests/sampler_paula_readers_test.c','src/editor/sampler_paula.c',
                 'src/core/sample_cache.c','src/core/playback_pcm.c','src/core/scheduled_readers.c',
                 'src/core/elapsed_clock.c','src/core/paula_render_voice.c',*SOURCES[1:]]
class PaulaReaders(unittest.TestCase):
    def test_genuine_persistent_master_and_chip_ownership(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core',*READERS_SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
