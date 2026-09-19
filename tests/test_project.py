from pathlib import Path
import subprocess
import tempfile
import unittest
import zlib
ROOT = Path(__file__).resolve().parents[1]
class Project(unittest.TestCase):
    def test_round_trip_bounds_and_staging(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'project-test')
            fixture = Path(tmp) / 'mixed.ptg'
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core', 'tests/project_test.c',
                            'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c',
                            '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary, str(fixture)], check=True)
            self.assertEqual(fixture.read_bytes(), (ROOT / 'tests/fixtures/project-v1/mixed.ptg').read_bytes())
            data = bytearray(fixture.read_bytes())
            expected = int.from_bytes(data[20:24], 'big')
            data[20:24] = bytes(4)
            self.assertEqual(zlib.crc32(data), expected)
