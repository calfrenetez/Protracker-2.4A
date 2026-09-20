from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class Document(unittest.TestCase):
    def test_allocation_and_save_faults(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'document-test')
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core', 'tests/document_test.c',
                            'src/core/document.c', 'src/core/pp20.c', 'src/core/safe_save.c', 'src/core/mod_project.c',
                            'src/core/mod_inspect.c', 'src/core/project.c', 'src/core/channels.c',
                            'src/core/pcm.c', '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary, str(ROOT / 'evidence/baseline/mod.baseline')], check=True)
