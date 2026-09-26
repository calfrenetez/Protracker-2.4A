from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES as SAMPLER
from test_render_invert_session import SOURCES as CORE
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['src/editor/sampler_invert_song.c',*dict.fromkeys([*[f'src/core/{s}.c' for s in CORE],*SAMPLER[1:]])]
class SamplerInvertSong(unittest.TestCase):
    def test_generation_and_private_lifetime(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/sampler_invert_song_test.c',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
