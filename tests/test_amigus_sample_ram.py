from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/amigus_sample_ram_test.c','src/core/amigus_sample_ram.c','src/core/sample_cache.c','src/core/playback_pcm.c','src/core/pcm.c']
class AmiGusSampleRam(unittest.TestCase):
    def test_bounded_device_cache(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=str(Path(tmp)/'test')
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',exe],cwd=ROOT,check=True)
            subprocess.run([exe],check=True)
