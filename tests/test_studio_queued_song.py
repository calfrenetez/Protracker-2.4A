from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class StudioQueuedSong(unittest.TestCase):
    def test_song_lifecycle(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'render'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',
                            'tests/studio_queued_song_test.c','src/core/studio_queue.c','src/core/studio_consumer.c','src/core/studio_pump.c','src/core/studio_song.c','src/core/studio_plan.c','src/core/studio_mix.c','src/core/render.c','src/core/pitch.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c',
                            'src/core/voice.c','src/core/project.c','src/core/channels.c','src/core/pcm.c','-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
