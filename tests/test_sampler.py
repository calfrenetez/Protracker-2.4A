from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/sampler_test.c','src/editor/sampler.c','src/core/pattern.c','src/core/document.c','src/core/project.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/channels.c','src/core/pcm.c','src/core/wav.c']
class Sampler(unittest.TestCase):
    def test_owned_versions_and_journal(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=str(Path(tmp)/'sampler')
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',binary],cwd=ROOT,check=True)
            subprocess.run([binary],check=True)
