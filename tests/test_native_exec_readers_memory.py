"""Execute the real Exec-backed reader fixture through the shared host model.

This checks allocator selection/accounting and the full lifecycle workload,
not native SDK layout, actual Fast/Chip placement, backend or device timing.
"""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from test_sampler_paula_readers import READERS_SOURCES
from test_native_exec_memory_markers import memory_model, POOL, FINISH
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import readers_memory_trace as trace
EXEC_READERS_SOURCES = ['tests/native_exec_sampler_paula_readers_test.c', *READERS_SOURCES[1:]]


class ExecReadersMemory(unittest.TestCase):
    def build(self, out, traced=False, component=0, source_binding=False):
        model = memory_model(out);binary = out/'readers-memory'
        flags = ['-DPT_READERS_DIAGNOSTIC_TRACE=1'] if traced else []
        if component:
            flags += ['-DPT_READERS_COMPONENT='+str(component)]
        if source_binding:
            flags += ['-DPT_READERS_SOURCE_BINDING_FIXTURE=1']
        build = subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
            '-fsanitize=address,undefined', '-DPT_TEST_NATIVE_MEMORY_FIXED_MARKERS=1', *flags,
            '-I'+str(out), '-Isrc/core', *EXEC_READERS_SOURCES, str(model), '-o', str(binary)],
            cwd=ROOT, check=True, capture_output=True, timeout=120)
        self.assertEqual((build.stdout, build.stderr), (b'', b''))
        return binary

    def test_source_binding_fixture_with_classified_host_memory(self):
        from test_sampler_paula_readers_source_binding import MARKER
        with tempfile.TemporaryDirectory(prefix='pt-readers-binding-memory-') as folder:
            binary=self.build(Path(folder),source_binding=True)
            result=subprocess.run([str(binary)],check=True,capture_output=True,timeout=30)
            self.assertEqual((result.stdout,result.stderr),
                (b'\n'.join([POOL,MARKER.encode('ascii'),
                 b'EXEC READERS CHIP PASS: Chip-only cache extents, zero owned bytes; modeled backend, no DMA or audio',
                 FINISH])+b'\n',b''))

    def test_source_binding_trace_and_final_closed_sidecar(self):
        from test_sampler_paula_readers_source_binding import MARKER
        self.assertEqual(MARKER,trace.BINDING)
        with tempfile.TemporaryDirectory(prefix='pt-readers-binding-trace-') as folder:
            out=Path(folder);binary=self.build(out,traced=True,source_binding=True)
            remote='RAM:PT24G-'+'c3'*8+'/progress.log';(out/remote).parent.mkdir()
            result=subprocess.run([str(binary),remote],cwd=out,check=True,capture_output=True,timeout=30)
            self.assertEqual((result.stdout,result.stderr),
                (('\n'.join(trace.markers('binding'))+'\n').encode('ascii'),b''))
            self.assertEqual(len(trace.markers('binding')),8)
            self.assertEqual((out/remote).read_bytes(),trace.FINAL_SIDECAR)

    def test_real_fixture_with_classified_host_memory_and_fixed_markers(self):
        with tempfile.TemporaryDirectory(prefix='pt-exec-readers-memory-') as folder:
            binary = self.build(Path(folder))
            result = subprocess.run([str(binary)], check=True, capture_output=True, timeout=90)
            self.assertEqual(result.stderr, b'')
            self.assertEqual(result.stdout.splitlines(), [POOL,
                b'SAMPLER PAULA READERS PASS: genuine master and selective Chip cache retained through 20 controls with command capacity2',
                b'EXEC READERS CHIP PASS: Chip-only cache extents, zero owned bytes; modeled backend, no DMA or audio', FINISH])

    def test_full_fixture_trace_and_final_closed_sidecar(self):
        with tempfile.TemporaryDirectory(prefix='pt-exec-readers-trace-') as folder:
            out = Path(folder);binary = self.build(out, traced=True)
            remote = 'RAM:PT24G-' + 'a1'*8 + '/progress.log'
            (out/remote).parent.mkdir()
            (out/remote).write_bytes(b'NOT STARTED\n')
            result = subprocess.run([str(binary), remote], cwd=out, check=True,
                                    capture_output=True, timeout=90)
            self.assertEqual(result.stderr, b'')
            self.assertEqual(result.stdout, ('\n'.join(trace.markers())+'\n').encode('ascii'))
            self.assertEqual((out/remote).read_bytes(), trace.FINAL_SIDECAR)
            self.assertLess(len(result.stdout), 8192)
            phases = [x for x in result.stdout.splitlines() if x.startswith(b'READERS MEMORY TRACE:')]
            self.assertEqual(len(phases), 92)
            self.assertEqual(len(list((out/remote).parent.iterdir())), 1)

    def test_separate_components_preserve_complete_lifecycle_and_fault_matrices(self):
        observed = {}
        for name, number in (('lifecycle',1),('faults',2)):
            with self.subTest(component=name), tempfile.TemporaryDirectory(prefix='pt-readers-component-') as folder:
                out=Path(folder);binary=self.build(out,traced=True,component=number)
                remote='RAM:PT24G-'+'b2'*8+'/progress.log';(out/remote).parent.mkdir()
                result=subprocess.run([str(binary),remote],cwd=out,check=True,capture_output=True,timeout=90)
                self.assertEqual(result.stderr,b'')
                self.assertEqual(result.stdout, ('\n'.join(trace.markers(name))+'\n').encode('ascii'))
                self.assertEqual((out/remote).read_bytes(),trace.FINAL_SIDECAR)
                observed[name]=[x for x in result.stdout.decode().splitlines() if x.startswith('READERS MEMORY TRACE:')]
        # Exclude the four common wrapper phases. Concatenated component bodies
        # preserve the complete full workload's original phase order.
        full=[x for x in trace.markers() if x.startswith('READERS MEMORY TRACE:')]
        self.assertEqual(observed['lifecycle'][2:-2]+observed['faults'][2:-2],full[2:-2])
        self.assertEqual((len(trace.markers('lifecycle')),len(trace.markers('faults'))),(80,24))
