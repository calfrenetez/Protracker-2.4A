from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class PaulaOutput(unittest.TestCase):
    def test_native_output_completion_and_storage_retention(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Itests/audio_stub','-Isrc/core',
                'tests/paula_output_test.c','-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
