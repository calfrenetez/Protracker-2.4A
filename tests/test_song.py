from pathlib import Path
import subprocess
import tempfile
import unittest
from test_editor import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class Song(unittest.TestCase):
    def test_growth_and_shared_history(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'song'
            sources=['tests/song_test.c',*[p for p in SOURCES if p not in ('tests/editor_test.c','src/editor/view.c')]]
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*sources,'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
