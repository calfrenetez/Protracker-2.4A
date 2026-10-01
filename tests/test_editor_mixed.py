from pathlib import Path
import subprocess,sys,tempfile,unittest
from test_mixed_owner import SOURCES as MIXED
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from build_editor_wavetable import prepare
class EditorMixed(unittest.TestCase):
    def test_indexed_editor_transport_lifetimes(self):
        with tempfile.TemporaryDirectory() as tmp:
            sources,tree=prepare(tmp);exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Isrc/core','tests/editor_mixed_test.c',
                *list(dict.fromkeys(['src/editor/editor_mixed.c',*MIXED[1:],*sources])),
                '-o',str(exe)],cwd=tmp,check=True)
            subprocess.run([str(exe)],check=True)
