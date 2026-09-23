from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class StudioMix(unittest.TestCase):
    def test_session(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','tests/studio_mix_test.c','src/core/studio_mix.c','src/core/voice.c','src/core/pcm.c','-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
