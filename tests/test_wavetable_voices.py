from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
from test_sampler_wavetable import EXTRA
ROOT=Path(__file__).resolve().parents[1]
class WavetableVoices(unittest.TestCase):
    def test_voice_lease_lifetime(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=str(Path(tmp)/'test')
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/wavetable_voices_test.c','src/editor/wavetable_voices.c',*EXTRA,*SOURCES[1:],'-o',exe],cwd=ROOT,check=True)
            subprocess.run([exe],check=True)
