from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class SamplerStudio(unittest.TestCase):
    def test_master_lifetime(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/sampler_studio_test.c','src/editor/sampler_studio.c','src/core/studio_mix.c','src/core/voice.c',*SOURCES[1:],'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
