from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler_prepare_memory import SOURCES as MEMORY
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/sampler_prepare_project_test.c','src/editor/sampler_prepare_project.c',*MEMORY[1:]]
class SamplerPrepareProject(unittest.TestCase):
    def test_borrowed_project_storage_before_child_ownership(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
