from pathlib import Path
import subprocess
import tempfile
import unittest
from test_sampler import ROOT,SOURCES
EXTRA=['src/editor/bounce.c','src/core/render.c','src/core/pitch.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c','src/core/voice.c']
class Bounce(unittest.TestCase):
    def test_atomic_generated_sample_and_shared_history(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'bounce'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/bounce_test.c',*SOURCES[1:],*EXTRA,'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
