from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
ROOT=Path(__file__).resolve().parents[1]
PREPARE_SOURCES=['tests/sampler_paula_readers_prepare_test.c','src/editor/sampler_paula.c',
                 'src/core/sample_cache.c','src/core/playback_pcm.c','src/core/scheduled_readers.c',
                 'src/core/elapsed_clock.c','src/core/paula_render_voice.c',*SOURCES[1:]]
class PaulaReadersPreparation(unittest.TestCase):
    def test_bounded_validation_and_checked_pool_transfer(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core',*PREPARE_SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
