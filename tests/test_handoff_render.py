from pathlib import Path
import subprocess,tempfile,unittest
from test_instrument_render import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class HandoffRender(unittest.TestCase):
    def test_reference_pcm_and_refusal(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/render_handoff_test.c',*SOURCES,'src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c','-o',str(exe)],cwd=ROOT,check=True)
            for name in ['loop','return','noloop','ed']:
                base=ROOT/'evidence/enhanced-editor/dev66/native'/('handoff_'+name)
                subprocess.run([str(exe),str(base)+'.mod',str(base)+'0.log','1' if name in ['loop','return'] else '0'],check=True)
