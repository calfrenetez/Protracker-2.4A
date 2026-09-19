from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class Midi(unittest.TestCase):
    def test_note_ownership_and_endpoint_lifecycle(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'midi-test')
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core', 'tests/midi_test.c',
                            'src/core/midi.c', '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True)
