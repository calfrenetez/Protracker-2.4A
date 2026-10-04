from pathlib import Path
import subprocess,tempfile,unittest
from test_editor import SOURCES,ROOT
class WorkflowView(unittest.TestCase):
    def test_planar_secondary_pages(self):
        with tempfile.TemporaryDirectory() as tmp:
            out=Path(tmp)/'workflow-view'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core','tests/workflow_view_test.c',
                            *SOURCES[1:],'-o',str(out)],cwd=ROOT,check=True)
            subprocess.run([str(out),str(ROOT/'vendor/pt23f/raw/ptfont.raw'),tmp],cwd=ROOT,check=True)
            for name in ('manager','toolbox','event-navigation','opened-master'):
                data=(Path(tmp)/(name+'.ppm')).read_bytes()
                self.assertEqual(data[:15],b'P6\n640 512\n255\n')
                self.assertEqual(len(data),15+640*512*3)
