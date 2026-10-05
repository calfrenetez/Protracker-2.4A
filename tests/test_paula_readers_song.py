from pathlib import Path
import subprocess
import tempfile
import unittest
from test_sampler_paula_readers_boundary import BOUNDARY_SOURCES

ROOT = Path(__file__).resolve().parents[1]
# The fixture includes the coordinator and inherited readers adapter exactly
# once. Neither included implementation C file is separately compiled.
SONG_SOURCES = ['tests/paula_readers_song_test.c', *BOUNDARY_SOURCES[1:]]


class PaulaReadersSong(unittest.TestCase):
    def test_real_whole_song_and_independent_domain_ownership(self):
        with tempfile.TemporaryDirectory() as tmp:
            executable = Path(tmp) / 'test'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-Isrc/core',
                *SONG_SOURCES, '-o', str(executable),
            ], cwd=ROOT, check=True)
            subprocess.run([str(executable)], check=True)
