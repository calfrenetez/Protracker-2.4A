from pathlib import Path
import subprocess,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from build_editor_wavetable import prepare
PAULA=['src/editor/editor_paula.c','src/editor/paula_song.c','src/core/elapsed_clock.c','src/editor/paula_dispatch.c',
    'src/editor/paula_voices.c','src/editor/sampler_paula.c','src/editor/paula_preflight.c',
    'src/core/paula_render_voice.c']
class EditorPaula(unittest.TestCase):
    def test_indexed_editor_lifetimes(self):
        with tempfile.TemporaryDirectory() as tmp:
            sources,tree=prepare(tmp);exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Isrc/core','tests/editor_paula_test.c',
                *list(dict.fromkeys([*PAULA,*sources])),'-o',str(exe)],cwd=tmp,check=True)
            subprocess.run([str(exe)],check=True)
