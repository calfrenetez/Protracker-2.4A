from pathlib import Path
import subprocess, tempfile, unittest
ROOT=Path(__file__).resolve().parents[1]
class PaulaPlayback(unittest.TestCase):
    def test_master_preserving_refusals(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=str(Path(tmp)/'test')
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core',
                            'tests/paula_playback_test.c','src/core/mod_project.c',
                            'src/core/mod_inspect.c','src/core/project.c',
                            'src/core/channels.c','src/core/pcm.c','-o',binary],cwd=ROOT,check=True)
            subprocess.run([binary],check=True)
