from pathlib import Path
import subprocess,tempfile,unittest
from test_mixed_preflight import SOURCES as PREFLIGHT
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/mixed_owner_test.c','src/editor/mixed_owner.c','src/editor/mixed_transport.c','src/editor/paula_voices.c','src/editor/sampler_paula.c','src/editor/paula_dispatch.c',*PREFLIGHT[1:]]
class MixedOwner(unittest.TestCase):
    def test_union_pins_and_both_reader_barrier(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
