from pathlib import Path
import subprocess
import tempfile
import unittest
from test_editor import ROOT,SOURCES
class Slots(unittest.TestCase):
    def test_sample_table_growth_and_shared_undo(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'slots'
            sources=['tests/slots_test.c',*[s for s in SOURCES if s not in ('tests/editor_test.c','src/editor/view.c')]]
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*sources,'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
