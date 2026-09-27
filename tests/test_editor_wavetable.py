from pathlib import Path
import subprocess,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from build_editor_wavetable import prepare
class EditorWavetable(unittest.TestCase):
    def test_staged_editor_lifetimes(self):
        with tempfile.TemporaryDirectory() as tmp:
            sources,tree=prepare(tmp);exe=Path(tmp)/'test'
            for fixture in ['editor_wavetable_test.c','editor_guard_test.c','editor_studio_test.c']:
                subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/'+fixture,*sources,'-o',str(exe)],cwd=tmp,check=True)
                subprocess.run([str(exe)],check=True)
