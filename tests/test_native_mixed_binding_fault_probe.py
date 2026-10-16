"""Genuine negative paths with exact stdout and a separately closed sidecar."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_sampler_mixed_source_binding import SOURCES as MIXED_SOURCES

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/native_mixed_binding_fault_probe_test.c', *MIXED_SOURCES[1:]]
PROBES = {
    'paula-before': {
        'selection': 1,
        'target': 'PTMixedPaulaFaultProbe',
        'markers': [
            'MIXED BINDING TRACE:entered:0:0:0',
            'MIXED BINDING TRACE:case-start:8:1:1',
            'MIXED BINDING TRACE:case-made:8:1:1',
            'MIXED BINDING TRACE:pool-open:8:1:1',
            'MIXED BINDING TRACE:begin:8:1:1',
            'MIXED BINDING TRACE:prepared-step:8:1:1',
            'MIXED BINDING TRACE:prepared-step:8:1:2',
            'MIXED BINDING TRACE:prepared-step:8:1:3',
            'MIXED BINDING TRACE:derived-ready:8:1:1',
            'MIXED BINDING TRACE:checking:8:1:1',
            'MIXED BINDING TRACE:checked:8:1:1',
            'MIXED BINDING TRACE:before-close:8:1:1',
            'MIXED BINDING TRACE:before-drop:8:1:1',
            'MIXED BINDING TRACE:case-closed:8:1:1',
            'MIXED BINDING FAULT PROBE PASS:one8mono Paula before-action wrong-sample refusal;genuine factory/cache/pins,unchanged masters and cleanup;other494 and legacy constructor excluded;SOFTWARE_ONLY',
            'MIXED BINDING TRACE:finished:8:1:1',
        ],
    },
    'amigus-active': {
        'selection': 2,
        'target': 'PTMixedAmigusFaultProbe',
        'markers': [
            'MIXED BINDING TRACE:entered:0:0:0',
            'MIXED BINDING TRACE:case-start:8:1:1',
            'MIXED BINDING TRACE:case-made:8:1:1',
            'MIXED BINDING TRACE:pool-open:8:1:1',
            'MIXED BINDING TRACE:derived-ready:8:1:1',
            'MIXED BINDING TRACE:checking:8:1:1',
            'MIXED BINDING TRACE:checked:8:1:1',
            'MIXED BINDING TRACE:before-close:8:1:1',
            'MIXED BINDING TRACE:before-drop:8:1:1',
            'MIXED BINDING TRACE:case-closed:8:1:1',
            'MIXED BINDING FAULT PROBE PASS:one8mono AmiGUS16le active wrong-sample refusal;genuine pins retained to independent retirement,unchanged masters and bus;other494 and legacy constructor excluded;SOFTWARE_ONLY',
            'MIXED BINDING TRACE:finished:8:1:1',
        ],
    },
}


class MixedBindingFaultProbe(unittest.TestCase):
    def check_probe(self, name):
        probe = PROBES[name]
        with tempfile.TemporaryDirectory(prefix='pt-mixed-fault-probe-') as folder:
            base = Path(folder)
            exe = base/'fixture'
            remote = 'RAM:PT24G-'+'ab'*8+'/progress.log'
            (base/remote).parent.mkdir()
            r = subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                '-fsanitize=address,undefined', '-Isrc/core', '-I.',
                '-DPT_MIXED_BINDING_BITS=8', '-DPT_MIXED_BINDING_CHANNELS=1',
                '-DPT_MIXED_FAULT_PROBE='+str(probe['selection']), *SOURCES, '-o', str(exe)],
                cwd=ROOT, capture_output=True, timeout=120)
            self.assertEqual((r.returncode, r.stdout, r.stderr), (0, b'', b''),
                             (r.stdout+r.stderr).decode('utf-8', 'replace'))
            r = subprocess.run([str(exe), remote], cwd=base, capture_output=True, timeout=30)
            self.assertEqual(r.returncode, 0, (r.stdout+r.stderr).decode('utf-8', 'replace'))
            self.assertEqual(r.stderr, b'')
            self.assertEqual(r.stdout.decode('ascii').splitlines(), probe['markers'])
            self.assertEqual((base/remote).read_bytes(), (probe['markers'][-1]+'\n').encode('ascii'))
            self.assertEqual([p.name for p in (base/remote).parent.iterdir()], ['progress.log'])

    def test_paula_before_action_wrong_sample(self):
        self.check_probe('paula-before')

    def test_amigus_active_wrong_sample_retains_until_independent_retirement(self):
        self.check_probe('amigus-active')


if __name__ == '__main__':
    unittest.main()
