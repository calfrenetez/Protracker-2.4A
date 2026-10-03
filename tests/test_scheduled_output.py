from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/scheduled_output_test.c','src/core/scheduled_output.c',
         'src/core/elapsed_clock.c','src/core/sample_cache.c']
class ScheduledOutput(unittest.TestCase):
    def test_future_capability_and_retirement(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
