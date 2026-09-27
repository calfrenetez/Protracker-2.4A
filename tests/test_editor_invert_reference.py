from pathlib import Path
import subprocess,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from build_editor_invert_reference import prepare
from make_invert_fixtures import four_clock_delay_fixtures
class EditorInvertReference(unittest.TestCase):
    def test_committed_editor_reference_ownership(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);sources=prepare(tmp);exe=tmp/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/editor_invert_reference_test.c',*sources,'-o',str(exe)],cwd=tmp,check=True)
            for name,data,meta in four_clock_delay_fixtures():
                mod=ROOT/'evidence/enhanced-editor/invert-four-clock-delay'/(name+'.mod')
                subprocess.run([str(exe),str(mod),str(mod.with_suffix('.trace')),str(meta['active_ticks'])],check=True)
