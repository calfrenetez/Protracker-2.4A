from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class WavImport(unittest.TestCase):
    def test_streamed_import(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            for faults in (False,True):
                subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',
                    *(['-Dread=pt_test_read','tests/render_read_faults.c'] if faults else []),
                    'tests/wav_import_test.c','src/platform/raw_import.c',*SOURCES[1:],'-o',str(exe)],cwd=ROOT,check=True)
                subprocess.run([str(exe),str(Path(tmp)/'sample.wav')],check=True)
