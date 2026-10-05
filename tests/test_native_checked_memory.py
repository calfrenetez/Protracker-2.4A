"""Host-only three-TU checked-memory ownership model."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CHECKED_MEMORY_SOURCES = [
    'tests/native_checked_memory_test.c',
    'src/native/native_checked_memory.c',
    'tests/checked_memory_host_stubs.c',
]


class CheckedMemorySoftwareModel(unittest.TestCase):
    def test_bounded_exact_ownership_and_refusals(self):
        with tempfile.TemporaryDirectory(prefix='pt-checked-memory-host-') as tmp:
            executable = Path(tmp) / 'checked-memory-test'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fno-omit-frame-pointer', '-fsanitize=address,undefined',
                '-DPT_CHECKED_MEMORY_HOST_ASAN=1',
                '-include', 'tests/checked_memory_host_stubs.h',
                '-Itests/checked_memory_host_sdk', '-Isrc/native',
                *CHECKED_MEMORY_SOURCES, '-o', str(executable),
            ], cwd=ROOT, check=True)
            subprocess.run([str(executable)], check=True)
