from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class InvertSequence(unittest.TestCase):
    def test_c_sequence_matches_pinned_mutations(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/invert_sequence_test.c','src/core/invert_sequence.c','src/core/invert_pcm.c','src/core/invert_loop.c','src/core/pcm.c','-o',str(exe)],cwd=ROOT,check=True)
            for name,speed,disable in [('fast',15,0),('slow',8,0),('disable',15,1)]:
                subprocess.run([str(exe),str(ROOT/f'evidence/enhanced-editor/dev91/native-reference/invert_{name}0.log'),str(speed),str(disable)],check=True)
