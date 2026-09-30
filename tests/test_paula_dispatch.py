from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
from test_paula_preflight import SOURCES as PREFLIGHT
ROOT=Path(__file__).resolve().parents[1]
class PaulaDispatch(unittest.TestCase):
    def test_whole_batch_and_failure_ownership(self):
        sources=list(dict.fromkeys(['tests/paula_dispatch_test.c','src/editor/paula_dispatch.c',
            'src/editor/paula_voices.c','src/editor/sampler_paula.c','src/core/sample_cache.c',
            'src/core/playback_pcm.c',*SOURCES[1:],*PREFLIGHT[1:]]))
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Isrc/core',*sources,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
