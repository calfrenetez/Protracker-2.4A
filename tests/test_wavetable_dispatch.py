from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
from test_sampler_wavetable import EXTRA
ROOT=Path(__file__).resolve().parents[1]
DISPATCH=['src/editor/wavetable_voices.c','src/editor/wavetable_dispatch.c','src/core/amigus_voice_plan.c','src/core/amigus_render_voice.c','src/core/render.c','src/core/voice.c','src/core/pitch.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c']
class WavetableDispatch(unittest.TestCase):
    def test_resolved_sequence_commands(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=str(Path(tmp)/'test')
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/wavetable_dispatch_test.c',*DISPATCH,*EXTRA,*SOURCES[1:],'-o',exe],cwd=ROOT,check=True)
            subprocess.run([exe],check=True)
