from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES as SAMPLER
from test_sampler_wavetable import EXTRA
from test_wavetable_dispatch import DISPATCH
from test_paula_preflight import SOURCES as PAULA
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/mixed_preflight_test.c','src/editor/mixed_preflight.c',*dict.fromkeys([*PAULA[1:],*DISPATCH,*EXTRA,*SAMPLER[1:]])]
class MixedPreflight(unittest.TestCase):
    def test_one_global_sequence_and_atomic_refusal(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
