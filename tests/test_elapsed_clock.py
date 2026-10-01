from pathlib import Path
import subprocess,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from build_editor_wavetable import prepare
class ElapsedClock(unittest.TestCase):
    def test_wide_product_oracle(self):
        with tempfile.TemporaryDirectory() as tmp:
            prepare(tmp);p=Path(tmp);exe=p/'elapsed'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','tests/elapsed_clock_fast_test.c',
                'src/core/elapsed_clock.c','-o',str(exe)],cwd=p,check=True)
            subprocess.run([str(exe)],check=True)
