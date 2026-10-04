from pathlib import Path
import subprocess
import tempfile
import unittest
from test_sampler_paula_readers import READERS_SOURCES

ROOT = Path(__file__).resolve().parents[1]
# The inherited fixture includes sampler_paula_readers.c exactly once. The
# remaining production units supply genuine sampler/cache and renderer behavior.
BOUNDARY_SOURCES = [
    'tests/sampler_paula_readers_boundary_test.c',
    *READERS_SOURCES[1:],
    'src/editor/paula_preflight.c',
    'src/core/render.c',
    'src/core/pitch.c',
    'src/core/timeline.c',
    'src/core/frame_clock.c',
    'src/core/flow.c',
    'src/core/voice.c',
]


class PaulaReadersBoundary(unittest.TestCase):
    def test_real_renderer_boundaries_and_independent_reader_ownership(self):
        with tempfile.TemporaryDirectory() as tmp:
            executable = Path(tmp) / 'test'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-Isrc/core',
                *BOUNDARY_SOURCES, '-o', str(executable),
            ], cwd=ROOT, check=True)
            subprocess.run([str(executable)], check=True)
