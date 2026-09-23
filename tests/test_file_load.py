from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class FileLoad(unittest.TestCase):
    def test_bounded_import(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp)
            for faults in (False,True):
                out=tmp/str(faults);out.mkdir();binary=tmp/'test'
                subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',
                    *(['-Dread=pt_test_read','tests/render_read_faults.c'] if faults else []),
                    'tests/file_load_test.c','src/platform/file_load.c','-o',str(binary)],cwd=ROOT,check=True)
                subprocess.run([str(binary),str(out)],check=True)
                self.assertEqual(list(out.iterdir()),[])
