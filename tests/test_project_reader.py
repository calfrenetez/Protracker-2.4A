from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class ProjectReader(unittest.TestCase):
    def test_bounded_preflight_matches_original(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=str(Path(tmp)/'reader')
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Isrc/core','tests/project_reader_test.c',
                'src/core/project.c','src/core/channels.c','src/core/pcm.c','-o',binary],cwd=ROOT,check=True)
            subprocess.run([binary,str(ROOT/'tests/fixtures/project-v1/mixed.ptg')],check=True)
