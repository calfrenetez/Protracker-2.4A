from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class RenderInvert(unittest.TestCase):
    def test_private_effect_render_and_failure_boundaries(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            sources=['render_invert','render','invert_bank','invert_sequence','invert_pcm','invert_loop','pitch','timeline','frame_clock','flow','voice','project','channels','pcm']
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/render_invert_test.c',*[f'src/core/{s}.c' for s in sources],'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
