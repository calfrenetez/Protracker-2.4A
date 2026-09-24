from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class InvertBank(unittest.TestCase):
    def test_bounded_private_bank_failure_and_reset(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/invert_bank_test.c','src/core/invert_bank.c','src/core/invert_pcm.c','src/core/invert_loop.c','src/core/pcm.c','-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
