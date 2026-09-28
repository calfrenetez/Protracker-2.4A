from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class AmiGusCapture(unittest.TestCase):
    def test_reserved_input_lifecycle(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'amigus-capture'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core','tests/amigus_capture_test.c',
                            'src/core/amigus_capture.c','src/core/amigus_reservation.c',
                            'src/core/amigus_interrupt_owner.c','src/core/capture_session.c',
                            'src/core/capture.c','src/editor/sampler_capture.c',*SOURCES[1:],'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
