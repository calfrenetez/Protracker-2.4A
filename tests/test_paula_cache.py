from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class PaulaCache(unittest.TestCase):
    def test_selective_snapshot_and_invalidation(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / 'cache-test')
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core',
                            'tests/paula_cache_test.c', 'src/core/mod_inspect.c',
                            '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True)
