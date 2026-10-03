from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['render_invert','render','invert_bank','invert_sequence','invert_pcm','invert_loop','pitch','timeline','frame_clock','flow','voice','project','channels','pcm']
class InvertPreparation(unittest.TestCase):
    def test_bounded_cancellable_initialization(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/render_invert_prepare_test.c',*[f'src/core/{s}.c' for s in SOURCES],'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
