from pathlib import Path
import subprocess,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from build_editor_wavetable import prepare
from test_editor_paula import PAULA
class NativeEditorPaula(unittest.TestCase):
    def test_prepared_device_barrier(self):
        with tempfile.TemporaryDirectory() as tmp:
            sources,tree=prepare(tmp);exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Itests/audio_stub','-Isrc/core',
                'tests/native_editor_paula_test.c',*list(dict.fromkeys([*PAULA,*sources])),
                '-o',str(exe)],cwd=tmp,check=True)
            subprocess.run([str(exe)],check=True)
