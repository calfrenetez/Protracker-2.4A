from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class StudioTick(unittest.TestCase):
    def test_interval_partition(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','tests/studio_tick_test.c','src/core/studio_tick.c','src/core/studio_mix.c','src/core/frame_clock.c','src/core/voice.c','src/core/pcm.c','-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
