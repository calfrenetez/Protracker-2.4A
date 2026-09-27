from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
ROOT=Path(__file__).resolve().parents[1]
EXTRA=['src/editor/sampler_wavetable.c']+['src/core/'+n+'.c' for n in ['amigus_reservation','amigus_wavetable_cache','amigus_sample_ram','sample_cache','playback_pcm']]
class SamplerWavetable(unittest.TestCase):
    def test_revision_and_master_lifetime(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=str(Path(tmp)/'test')
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/sampler_wavetable_test.c',*EXTRA,*SOURCES[1:],'-o',exe],cwd=ROOT,check=True)
            subprocess.run([exe],check=True)
