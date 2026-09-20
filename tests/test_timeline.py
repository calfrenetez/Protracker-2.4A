from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class Timeline(unittest.TestCase):
    def test_flow_and_frame_clock_are_one_transaction(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'timeline'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',
                            'tests/timeline_test.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c',
                            'src/core/project.c','src/core/channels.c','src/core/pcm.c','-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
