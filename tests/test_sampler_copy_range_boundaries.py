from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler_workflow import SOURCES as WORKFLOW_SOURCES
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/sampler_copy_range_boundaries_test.c',*WORKFLOW_SOURCES[1:]]
class SamplerCopyRangeBoundaries(unittest.TestCase):
    def test_private_copy_failure_boundaries(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'sampler-copy-boundaries'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
