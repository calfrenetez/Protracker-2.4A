"""HOST execution of actual one-control probe and its closed trace contract."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_sampler_mixed_source_binding import SOURCES as MIXED_SOURCES
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/native_mixed_binding_probe_test.c',*MIXED_SOURCES[1:]]
MARKERS=[
    'MIXED BINDING TRACE:entered:0:0:0',
    'MIXED BINDING TRACE:case-start:8:1:0',
    'MIXED BINDING TRACE:case-made:8:1:0',
    'MIXED BINDING TRACE:pool-open:8:1:0',
    'MIXED BINDING TRACE:begin:8:1:0',
    'MIXED BINDING TRACE:prepared-step:8:1:1',
    'MIXED BINDING TRACE:prepared-step:8:1:2',
    'MIXED BINDING TRACE:prepared-step:8:1:3',
    'MIXED BINDING TRACE:derived-ready:8:1:0',
    'MIXED BINDING TRACE:checking:8:1:0',
    'MIXED BINDING TRACE:checked:8:1:0',
    'MIXED BINDING TRACE:before-close:8:1:0',
    'MIXED BINDING TRACE:before-drop:8:1:0',
    'MIXED BINDING TRACE:case-closed:8:1:0',
    'MIXED BINDING PROBE PASS:one8mono Paula before-action control;genuine factory/cache/pins and cleanup;other494 cases and legacy constructor excluded;SOFTWARE_ONLY',
    'MIXED BINDING TRACE:finished:8:1:1',
]
class MixedBindingProbe(unittest.TestCase):
    def test_actual_probe_phases_and_final_closed_sidecar(self):
        with tempfile.TemporaryDirectory(prefix='pt-mixed-probe-') as folder:
            base=Path(folder);exe=base/'fixture';remote='RAM:PT24G-'+'ab'*8+'/progress.log'
            (base/remote).parent.mkdir()
            r=subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-UNDEBUG',
                '-fsanitize=address,undefined','-Isrc/core','-I.',
                '-DPT_MIXED_BINDING_BITS=8','-DPT_MIXED_BINDING_CHANNELS=1',*SOURCES,'-o',str(exe)],
                cwd=ROOT,capture_output=True,timeout=120)
            self.assertEqual(r.returncode,0,(r.stdout+r.stderr).decode('utf-8','replace'))
            self.assertEqual((r.stdout,r.stderr),(b'',b''))
            r=subprocess.run([str(exe),remote],cwd=base,capture_output=True,timeout=30)
            self.assertEqual(r.returncode,0,(r.stdout+r.stderr).decode('utf-8','replace'))
            self.assertEqual(r.stderr,b'')
            self.assertEqual(r.stdout.decode('ascii').splitlines(),MARKERS)
            self.assertEqual((base/remote).read_bytes(),(MARKERS[-1]+'\n').encode('ascii'))
            self.assertEqual([p.name for p in (base/remote).parent.iterdir()],['progress.log'])
if __name__=='__main__':unittest.main()
