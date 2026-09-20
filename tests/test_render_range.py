from pathlib import Path
import subprocess,tempfile,unittest
from test_render_file import RENDER
ROOT=Path(__file__).resolve().parents[1]
class RenderRange(unittest.TestCase):
    def test_preroll_crop_and_boundaries(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'range'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/render_range_test.c',*RENDER,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
            # A 44.1 kHz source leaves a nonzero fractional/loop phase after
            # pre-roll, unlike the integral-rate fixture's whole-loop boundary.
            variant=Path(tmp)/'fractional.c'
            source=(ROOT/'tests/render_range_test.c').read_text()
            self.assertEqual(source.count('sample.pcm.rate=48000'),1)
            variant.write_text(source.replace('sample.pcm.rate=48000','sample.pcm.rate=44100'))
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',str(variant),*RENDER,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
