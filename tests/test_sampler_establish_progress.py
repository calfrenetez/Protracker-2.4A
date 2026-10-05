from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler_establish import SOURCES as ORIGINAL
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/sampler_establish_progress_test.c',*ORIGINAL[1:]]
class ObservableEstablishment(unittest.TestCase):
    def test_complete_assertion_workload_with_flushed_boundaries(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
