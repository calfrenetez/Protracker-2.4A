from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class RenderInvertFile(unittest.TestCase):
    def test_verified_private_effect_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);exe=tmp/'test';output=tmp/'out';output.mkdir()
            sources=['stems','render_invert','render','invert_bank','invert_sequence','invert_pcm','invert_loop','pitch','timeline','frame_clock','flow','voice','project','channels','pcm']
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','-Isrc/platform','tests/render_invert_file_test.c','src/platform/render_file.c','src/platform/stem_file.c','src/platform/render_invert_file.c',*[f'src/core/{s}.c' for s in sources],'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe),str(output/'test.wav')],check=True)
            self.assertEqual(list(output.iterdir()),[])

    def test_verified_shared_sample_stems(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);exe=tmp/'test';output=tmp/'out';output.mkdir()
            sources=['stems','render_invert','render','invert_bank','invert_sequence','invert_pcm','invert_loop','pitch','timeline','frame_clock','flow','voice','project','channels','pcm']
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','-Isrc/platform','tests/render_invert_stems_test.c','src/platform/render_file.c','src/platform/stem_file.c','src/platform/render_invert_file.c',*[f'src/core/{s}.c' for s in sources],'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe),str(output/'stems')],check=True)
            self.assertEqual(list(output.iterdir()),[])
