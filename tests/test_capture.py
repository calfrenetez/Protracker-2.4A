from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class Capture(unittest.TestCase):
    def test_staging_and_publication(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'capture'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core','tests/capture_test.c',
                            'src/core/capture.c','src/editor/sampler_capture.c',*SOURCES[1:],
                            '-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
