from pathlib import Path
import subprocess,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from generate_pitch_tables import generated
class FinetuneCore(unittest.TestCase):
    def test_tables_and_state(self):
        self.assertEqual((ROOT/'src/core/pitch_tables.h').read_text(),generated())
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'fine'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/finetune_test.c','src/core/pitch.c','src/core/channels.c','-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
