from pathlib import Path
import subprocess,tempfile,unittest
from test_mixed_owner_established import SOURCES as MIXED
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/sampler_establish_test.c','src/editor/sampler_establish.c',*MIXED[1:]]
class EstablishMasters(unittest.TestCase):
    def test_validation_then_guarded_atomic_masters_before_mixed_borrow(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
