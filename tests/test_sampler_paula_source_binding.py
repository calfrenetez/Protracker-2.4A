from pathlib import Path
import subprocess
import tempfile
import unittest
from test_sampler import SOURCES

ROOT = Path(__file__).resolve().parents[1]


class SamplerPaulaSourceBinding(unittest.TestCase):
    def test_genuine_source_and_representation_pairing(self):
        with tempfile.TemporaryDirectory() as directory:
            exe = Path(directory) / 'source-binding'
            subprocess.run(['/usr/bin/cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra',
                            '-Werror', '-UNDEBUG', '-fsanitize=address,undefined', '-Isrc/core',
                            'tests/sampler_paula_source_binding_test.c', 'src/editor/sampler_paula.c',
                            'src/core/sample_cache.c', 'src/core/playback_pcm.c',
                            *SOURCES[1:], '-o', str(exe)], cwd=ROOT, check=True)
            result = subprocess.run([str(exe)], capture_output=True, timeout=30)
            self.assertEqual((result.returncode, result.stderr), (0, b''))
            self.assertEqual(len(result.stdout.splitlines()), 3)
            self.assertTrue(result.stdout.endswith(
                b'PAULA SOURCE BINDING PASS: 57 source/lease checks, 36 representation checks; '
                b'8/16/24 masters, explicit stereo side, no lookup mutation, zero owned resources\n'))
            print(result.stdout.decode('ascii'), end='')


if __name__ == '__main__':
    unittest.main()
