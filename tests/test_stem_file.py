from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
from test_render_file import RENDER
class StemFile(unittest.TestCase):
    def test_batch_publication(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);exe=tmp/'stems';out=tmp/'out';out.mkdir()
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/stem_file_test.c',*RENDER,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe),str(out)],check=True)
            self.assertEqual(sorted(p.name for p in out.iterdir()),['batch','race','single-0.wav','single-1.wav','single-2.wav'])
            self.assertEqual(list((out/'race').iterdir()),[])

            allocated_out=tmp/'allocated';allocated_out.mkdir()
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/stem_file_alloc_test.c',*RENDER,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe),str(allocated_out)],check=True)
            self.assertEqual(sorted(p.name for p in allocated_out.iterdir()),['batch','race','single-0.wav','single-1.wav','single-2.wav'])
