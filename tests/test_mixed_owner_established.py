from pathlib import Path
import subprocess,tempfile,unittest
from test_mixed_owner import SOURCES as LEGACY
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/mixed_owner_established_test.c','src/editor/mixed_owner_established.c','src/editor/sampler_prepare_project.c','src/editor/sampler_prepare_memory.c',*LEGACY[1:]]
class EstablishedMixedOwner(unittest.TestCase):
    def test_checked_sequence_retains_established_masters(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
