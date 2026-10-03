from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
ROOT=Path(__file__).resolve().parents[1]
FUTURE_SOURCES=['tests/sampler_paula_future_test.c','src/editor/sampler_paula_future.c',
                'src/editor/sampler_paula.c','src/core/sample_cache.c','src/core/playback_pcm.c',
                'src/core/scheduled_output.c','src/core/elapsed_clock.c',*SOURCES[1:]]
class PaulaFuture(unittest.TestCase):
    def test_independent_trigger_resources_and_retirement(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core',*FUTURE_SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
