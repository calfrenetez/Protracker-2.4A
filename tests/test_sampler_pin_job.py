from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class SamplerPinJob(unittest.TestCase):
    def test_bounded_promotion(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=str(Path(tmp)/'pin-job')
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/sampler_pin_job_test.c',*SOURCES[1:],'-o',binary],cwd=ROOT,check=True)
            subprocess.run([binary],check=True)
