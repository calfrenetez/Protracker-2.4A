from pathlib import Path
import subprocess
import tempfile
import unittest
from test_editor import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class SampleDispatch(unittest.TestCase):
    def test_bounded_dispatch_and_precision(self):
        sources=['tests/sample_dispatch_test.c','src/platform/sample_import.c','src/platform/raw_import.c','src/platform/mod_import.c','src/platform/pp20_import.c',*[p for p in SOURCES if p not in ('tests/editor_test.c','src/editor/view.c')]]
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'dispatch'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*sources,'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary),str(Path(tmp)/'input')],check=True)
