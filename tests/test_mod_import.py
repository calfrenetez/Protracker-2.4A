from pathlib import Path
import subprocess,tempfile,unittest
from test_mod_round8 import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class ModImport(unittest.TestCase):
    def test_streaming_document(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            for faults in (False,True):
                subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',
                    *(['-Dread=pt_test_read','tests/render_read_faults.c'] if faults else []),
                    'tests/mod_import_test.c','src/platform/mod_import.c',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
                subprocess.run([str(exe),str(Path(tmp)/'input.mod')],check=True)
