from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/amigus_wavetable_cache_test.c']+['src/core/'+n+'.c' for n in ['amigus_reservation','amigus_wavetable_cache','amigus_sample_ram','sample_cache','playback_pcm','pcm']]
class WavetableOwner(unittest.TestCase):
    def test_resource_and_cache_lifetime(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=str(Path(tmp)/'test')
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',exe],cwd=ROOT,check=True)
            subprocess.run([exe],check=True)
