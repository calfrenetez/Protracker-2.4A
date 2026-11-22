from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class RoutedRenderSequence(unittest.TestCase):
    def test_mixed_routes_share_the_render_timeline(self):
        sources = ['tests/routed_render_sequence_test.c'] + [
            f'src/core/{name}.c' for name in
            ('render', 'pitch', 'timeline', 'frame_clock', 'flow', 'voice',
             'project', 'channels', 'pcm', 'midi', 'routed_midi')
        ]
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'routed'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-Isrc/core', *sources,
                '-o', str(binary),
            ], cwd=ROOT, check=True, timeout=120)
            subprocess.run([str(binary)], check=True, timeout=15)
