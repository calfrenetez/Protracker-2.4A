from pathlib import Path
import subprocess
import tempfile
import unittest
from test_paula_readers_song import SONG_SOURCES

ROOT = Path(__file__).resolve().parents[1]
# The new TU includes the genuine coordinator and inherited readers adapter.
# Their included C implementations are not separately compiled. The existing
# song fixture/recipe remains byte-exact and runs as its separate legacy group.
WORKSPACE_SOURCES = ['tests/paula_readers_song_workspace_test.c', *SONG_SOURCES[1:]]


class PaulaReadersSongWorkspace(unittest.TestCase):
    def test_guarded_caller_workspace_and_real_exact_song_lifecycle(self):
        with tempfile.TemporaryDirectory() as tmp:
            executable = Path(tmp) / 'test'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-Isrc/core',
                *WORKSPACE_SOURCES, '-o', str(executable),
            ], cwd=ROOT, check=True)
            subprocess.run([str(executable)], check=True)
