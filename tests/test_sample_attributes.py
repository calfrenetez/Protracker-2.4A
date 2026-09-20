from pathlib import Path
import subprocess
import tempfile
import unittest
from test_editor import ROOT,SOURCES

class SampleAttributes(unittest.TestCase):
    def test_shared_pcm_and_history(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=str(Path(tmp)/'sample-attributes')
            sources=['tests/sample_attributes_test.c',*[s for s in SOURCES if s not in ('tests/editor_test.c','src/editor/view.c')]]
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*sources,'-o',binary],cwd=ROOT,check=True)
            subprocess.run([binary],check=True)
