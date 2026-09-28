from pathlib import Path
import subprocess,tempfile,unittest
from test_editor import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class EditorCapture(unittest.TestCase):
    def test_recording_barrier_and_publication(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'editor-capture'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core','tests/editor_capture_test.c',
                            'src/editor/editor_capture.c','src/core/amigus_capture.c',
                            'src/core/amigus_reservation.c','src/core/amigus_interrupt_owner.c',
                            'src/core/capture_session.c','src/core/capture.c','src/editor/sampler_capture.c',
                            *[s for s in SOURCES[1:] if s!='src/editor/view.c'],'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
