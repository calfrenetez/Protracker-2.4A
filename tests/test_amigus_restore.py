from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/amigus_restore_test.c','src/core/amigus_render_voice.c','src/core/voice.c','src/core/pcm.c']
class AmiGusRestore(unittest.TestCase):
    def test_exact_restore_plan(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=str(Path(tmp)/'test')
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',exe],cwd=ROOT,check=True)
            subprocess.run([exe],check=True)
