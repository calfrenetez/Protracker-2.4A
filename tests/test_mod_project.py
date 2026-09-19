from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class ModProject(unittest.TestCase):
    def test_lossless_classic_round_trip_and_loss_reports(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'mod-project-test')
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core', 'tests/mod_project_test.c',
                            'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/project.c',
                            'src/core/channels.c', 'src/core/pcm.c', '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary, str(ROOT / 'evidence/baseline/mod.baseline')], check=True)
