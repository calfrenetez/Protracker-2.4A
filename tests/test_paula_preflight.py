from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/paula_preflight_test.c','src/editor/paula_preflight.c','src/core/paula_render_voice.c',
         'src/core/render.c','src/core/pitch.c','src/core/timeline.c','src/core/frame_clock.c',
         'src/core/flow.c','src/core/voice.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
class PaulaPreflight(unittest.TestCase):
    def test_shared_timeline_and_explicit_capabilities(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
