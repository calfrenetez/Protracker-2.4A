from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class Raw(unittest.TestCase):
    def test_codec(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=str(Path(tmp)/'raw')
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/raw_test.c','src/core/raw.c','src/core/pcm.c','-o',binary],cwd=ROOT,check=True)
            subprocess.run([binary],check=True)
