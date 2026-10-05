from pathlib import Path
import subprocess,tempfile,unittest
from test_mixed_preflight import SOURCES as MIXED
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/sampler_prepare_memory_test.c','src/editor/sampler_prepare_memory.c',*MIXED[1:]]
class SamplerPrepareMemory(unittest.TestCase):
    def test_real_sampler_storage_and_checked_mixed_children(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
