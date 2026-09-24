from pathlib import Path
import subprocess,tempfile,unittest
from test_editor import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class EditorGuard(unittest.TestCase):
    def test_before_mutation(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'guard'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/editor_guard_test.c',*SOURCES[1:],'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
