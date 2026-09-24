from pathlib import Path
import subprocess,tempfile,unittest
from test_editor import SOURCES
ROOT=Path(__file__).resolve().parents[1]
EXTRA=['src/core/amigus_session.c','src/core/amigus_fifo.c','src/core/amigus_pcm_pack.c','src/core/studio_consumer.c','src/core/studio_pump.c','src/core/studio_queue.c','src/editor/editor_studio.c','src/editor/sampler_song.c','src/editor/sampler_studio.c','src/core/studio_song.c','src/core/studio_plan.c','src/core/render.c','src/core/pitch.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c','src/core/studio_mix.c','src/core/voice.c']
class EditorStudio(unittest.TestCase):
    def test_pinned_editor_lifecycle(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/editor_studio_test.c',*EXTRA,*SOURCES[1:],'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
