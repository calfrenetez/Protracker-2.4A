from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES as SAMPLER_SOURCES
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/sampler_workflow_test.c','src/core/sample_usage.c',*SAMPLER_SOURCES[1:],'src/editor/sampler_paula.c','src/core/sample_cache.c','src/core/playback_pcm.c']
class SamplerWorkflow(unittest.TestCase):
    def test_atomic_cleanup_and_range_copy(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'sampler-workflow'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','-DPT_SAMPLER_WORKFLOW_PROFILE',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
