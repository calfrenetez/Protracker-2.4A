from pathlib import Path
import hashlib
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/editor_test.c', 'src/editor/editor.c', 'src/editor/view.c', 'src/editor/sampler.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/wav.c', 'src/core/svx.c',
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
            # Reference-refinement golden; incremental redraws must match it exactly.
            self.assertEqual(hashlib.sha256((Path(tmp)/'editor.ppm').read_bytes()).hexdigest(),
                             '87ebf44ea470be133f7d47931f04210e5b883b90e3b882a256f1f641bd7450eb')
