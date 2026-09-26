from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import ROOT,SOURCES
from test_bounce import EXTRA
INVERT=['src/editor/bounce_invert.c','src/core/render_invert.c','src/core/invert_bank.c','src/core/invert_sequence.c','src/core/invert_pcm.c','src/core/invert_loop.c']
class BounceInvert(unittest.TestCase):
    def test_shared_sample_bounce_transaction(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'bounce'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/bounce_invert_test.c',*SOURCES[1:],*EXTRA,*INVERT,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
