from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class AmiGusVoicePlan(unittest.TestCase):
    def test_plan_boundaries(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=str(Path(tmp)/'test')
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/amigus_voice_plan_test.c','src/core/amigus_voice_plan.c','-o',exe],cwd=ROOT,check=True)
            subprocess.run([exe],check=True)
