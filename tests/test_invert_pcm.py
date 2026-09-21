from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class InvertPCM(unittest.TestCase):
    def test_private_shared_mutations_reset_and_refusal(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/invert_pcm_test.c','src/core/invert_pcm.c','src/core/invert_loop.c','src/core/pcm.c','-o',str(exe)],cwd=ROOT,check=True)
            self.assertIn('INVERT private PCM PASS',subprocess.check_output([str(exe)],text=True))
