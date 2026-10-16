"""Execute the real Exec-backed reader fixture through the shared host model.

This checks allocator selection/accounting and the full lifecycle workload,
not native SDK layout, actual Fast/Chip placement, backend or device timing.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_sampler_paula_readers import READERS_SOURCES
from test_native_exec_memory_markers import memory_model, POOL, FINISH
ROOT = Path(__file__).resolve().parents[1]
EXEC_READERS_SOURCES = ['tests/native_exec_sampler_paula_readers_test.c', *READERS_SOURCES[1:]]


class ExecReadersMemory(unittest.TestCase):
    def test_real_fixture_with_classified_host_memory_and_fixed_markers(self):
        with tempfile.TemporaryDirectory(prefix='pt-exec-readers-memory-') as folder:
            out = Path(folder);model = memory_model(out);binary = out/'readers-memory'
            build = subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-DPT_TEST_NATIVE_MEMORY_FIXED_MARKERS=1',
                '-I'+str(out), '-Isrc/core', *EXEC_READERS_SOURCES, str(model), '-o', str(binary)],
                cwd=ROOT, check=True, capture_output=True, timeout=120)
            self.assertEqual((build.stdout, build.stderr), (b'', b''))
            result = subprocess.run([str(binary)], check=True, capture_output=True, timeout=90)
            self.assertEqual(result.stderr, b'')
            self.assertEqual(result.stdout.splitlines(), [POOL,
                b'SAMPLER PAULA READERS PASS: genuine master and selective Chip cache retained through 20 controls with command capacity2',
                b'EXEC READERS CHIP PASS: Chip-only cache extents, zero owned bytes; modeled backend, no DMA or audio', FINISH])
