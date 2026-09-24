from pathlib import Path
import subprocess
import tempfile
import unittest
from test_mod_round8 import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class ProjectImport(unittest.TestCase):
    def test_streamed_transaction(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=str(Path(tmp)/'import')
            sources=['tests/project_import_test.c','src/platform/project_import.c',*SOURCES]
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*sources,'-o',binary],cwd=ROOT,check=True)
            subprocess.run([binary,str(ROOT/'tests/fixtures/project-v1/mixed.ptg')],check=True)
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Dread=pt_test_read','-Isrc/core',
                *sources,'tests/render_read_faults.c','-o',binary],cwd=ROOT,check=True)
            subprocess.run([binary,str(ROOT/'tests/fixtures/project-v1/mixed.ptg')],check=True)
