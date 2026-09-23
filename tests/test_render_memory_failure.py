from pathlib import Path
import subprocess,tempfile,unittest
from test_render_file import ROOT,RENDER
class RenderMemoryFailure(unittest.TestCase):
    def test_every_export_allocation_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);binary=tmp/'test';out=tmp/'out';out.mkdir()
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                            '-Isrc/core','tests/render_memory_failure_test.c',*RENDER,'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary),str(out)],check=True)
            self.assertEqual(list(out.iterdir()),[])
