from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = [
    'tests/render_sequence_startup_test.c',
    'src/core/render.c', 'src/core/pitch.c', 'src/core/timeline.c',
    'src/core/frame_clock.c', 'src/core/flow.c', 'src/core/voice.c',
    'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c',
]


class RenderSequenceStartup(unittest.TestCase):
    def test_actual_cancellable_setup_and_checked_reset(self):
        with tempfile.TemporaryDirectory() as tmp:
            executable = Path(tmp) / 'render-startup'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-Isrc/core',
                *SOURCES, '-o', str(executable),
            ], cwd=ROOT, check=True)
            subprocess.run([str(executable)], check=True)
