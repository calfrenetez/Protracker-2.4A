from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class Stems(unittest.TestCase):
    def test_group_partition_and_refusal(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'stems'
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-O1','-g','-fsanitize=address,undefined','-Isrc/core','tests/stems_test.c','src/core/stems.c','src/core/channels.c','-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
