from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class SampleCache(unittest.TestCase):
    def test_ownership_versions_pressure_and_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'sample-cache-test')
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core',
                            'tests/sample_cache_test.c', 'src/core/sample_cache.c',
                            '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True)
