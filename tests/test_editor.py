from pathlib import Path
import hashlib
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/editor_test.c', 'src/editor/editor.c', 'src/editor/view.c',
           'src/core/document.c', 'src/core/pattern.c', 'src/core/project.c',
           'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c']
class Editor(unittest.TestCase):
    def test_native_shared_controller_and_planar_display(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'editor-test')
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core', *SOURCES, '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary, str(ROOT/'tests/fixtures/project-v1/mixed.ptg'),
                            str(ROOT/'vendor/pt23f/raw/ptfont.raw'), str(Path(tmp)/'editor.ppm')], check=True)
            # Golden pixels were captured with the pre-optimization renderer.
            self.assertEqual(hashlib.sha256((Path(tmp)/'editor.ppm').read_bytes()).hexdigest(),
                             'b34905893d4d58612dfced2acf7f67649387c9a184bfcbbecdeb059526d2b9af')
