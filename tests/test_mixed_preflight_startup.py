from pathlib import Path
import subprocess
import tempfile
import unittest
from test_mixed_preflight import SOURCES as MIXED_SOURCES

ROOT = Path(__file__).resolve().parents[1]
# The fixture includes only genuine renderer helpers with renamed main.
# Existing mixed closure supplies each production translation unit once.
SOURCES = ['tests/mixed_preflight_startup_test.c', *MIXED_SOURCES[1:]]


class MixedPreflightStartup(unittest.TestCase):
    def test_initial_readiness_then_complete_two_route_audit(self):
        with tempfile.TemporaryDirectory() as tmp:
            executable = Path(tmp) / 'mixed-startup'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-Isrc/core',
                *SOURCES, '-o', str(executable),
            ], cwd=ROOT, check=True)
            subprocess.run([str(executable)], check=True)
