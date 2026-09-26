from pathlib import Path
import subprocess,tempfile,unittest
from test_editor import SOURCES
from test_editor_studio import EXTRA
ROOT=Path(__file__).resolve().parents[1]
class EditorInvertStudio(unittest.TestCase):
    def test_private_editor_lifecycle(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/editor_invert_studio_test.c',*EXTRA,*SOURCES[1:],'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
