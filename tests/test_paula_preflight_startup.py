from pathlib import Path
import subprocess
import tempfile
import unittest
from test_render_sequence_startup import SOURCES as RENDER_STARTUP_SOURCES

ROOT = Path(__file__).resolve().parents[1]
# The Paula fixture includes the renderer fixture's genuine project/allocator
# helpers with a renamed main, without separately compiling that fixture.
SOURCES = [
    'tests/paula_preflight_startup_test.c',
    *RENDER_STARTUP_SOURCES[1:],
    'src/editor/paula_preflight.c', 'src/core/paula_render_voice.c',
]


class PaulaPreflightStartup(unittest.TestCase):
    def test_initial_readiness_then_actual_timeline_audit(self):
        with tempfile.TemporaryDirectory() as tmp:
            executable = Path(tmp) / 'paula-startup'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-Isrc/core',
                *SOURCES, '-o', str(executable),
            ], cwd=ROOT, check=True)
            subprocess.run([str(executable)], check=True)
