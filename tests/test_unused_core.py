from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class UnusedCore(unittest.TestCase):
    def test_pinned_unused_command_semantics(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'unused'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/unused_test.c','src/core/pitch.c','src/core/channels.c','-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
