from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = [
    'tests/studio_master_outputs_test.c', 'src/core/studio_mix.c',
    'src/core/voice.c', 'src/core/pcm.c',
]
MARKER = 'STUDIO MASTER OUTPUT PASS:48 refusals/48 controls;8/16/24 mono/stereo;current/pending full capacity and opaque owner;phase/pins/bytes preserved\n'


class StudioMasterOutputs(unittest.TestCase):
    def test_pinned_master_outputs(self):
        with tempfile.TemporaryDirectory() as folder:
            binary = Path(folder) / 'test'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', *SOURCES, '-o', str(binary),
            ], cwd=ROOT, check=True)
            result = subprocess.run([str(binary)], capture_output=True, text=True, check=True)
            self.assertEqual(result.stdout, MARKER)
            self.assertEqual(result.stderr, '')


if __name__ == '__main__':
    unittest.main()
