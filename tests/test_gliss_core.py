from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class GlissCore(unittest.TestCase):
    def test_independent_glissando_controls(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'gliss'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/gliss_test.c','src/core/pitch.c','src/core/channels.c','-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
