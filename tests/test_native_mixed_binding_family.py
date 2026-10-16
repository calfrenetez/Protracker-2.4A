"""Explicit backend families partition the original75 genuine8mono cases."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_sampler_mixed_source_binding import SOURCES as MIXED_SOURCES

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/native_mixed_binding_family_test.c', *MIXED_SOURCES[1:]]
FAMILIES = {'paula': 0, 'amigus8be': 1, 'amigus8le': 2, 'amigus16be': 3, 'amigus16le': 4}


def family_markers(name):
    assert name in FAMILIES
    markers = ['MIXED BINDING TRACE:family-entered:8:1:0']
    for fault in range(5):
        for phase in range(3):
            markers += ['MIXED BINDING TRACE:family-case-start:8:'+str(phase)+':'+str(fault),
                        'MIXED BINDING TRACE:family-case-closed:8:'+str(phase)+':'+str(fault)]
    markers += ['MIXED BINDING FAMILY PASS:8mono '+name+
                ';15 cases;3 controls12 refusals;before-action/enqueue/active readiness;genuine pins and independent retirement;other480 and legacy constructor excluded;SOFTWARE_ONLY',
                'MIXED BINDING TRACE:family-finished:8:1:15']
    return markers


class MixedBindingFamilies(unittest.TestCase):
    def test_all_five_genuine_backend_families_and_closed_sidecar(self):
        for name, selection in FAMILIES.items():
            with self.subTest(family=name), tempfile.TemporaryDirectory(prefix='pt-mixed-family-') as folder:
                base = Path(folder)
                exe = base/'fixture'
                remote = 'RAM:PT24G-'+'ab'*8+'/progress.log'
                (base/remote).parent.mkdir()
                r = subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                    '-fsanitize=address,undefined', '-Isrc/core', '-I.',
                    '-DPT_MIXED_BINDING_BITS=8', '-DPT_MIXED_BINDING_CHANNELS=1',
                    '-DPT_MIXED_FAMILY='+str(selection), *SOURCES, '-o', str(exe)],
                    cwd=ROOT, capture_output=True, timeout=120)
                self.assertEqual((r.returncode, r.stdout, r.stderr), (0, b'', b''),
                                 (r.stdout+r.stderr).decode('utf-8', 'replace'))
                r = subprocess.run([str(exe), remote], cwd=base, capture_output=True, timeout=30)
                self.assertEqual(r.returncode, 0, (r.stdout+r.stderr).decode('utf-8', 'replace'))
                self.assertEqual(r.stderr, b'')
                markers = family_markers(name)
                self.assertEqual(len(markers), 33)
                self.assertEqual(r.stdout.decode('ascii').splitlines(), markers)
                self.assertEqual((base/remote).read_bytes(), (markers[-1]+'\n').encode('ascii'))
                self.assertEqual([p.name for p in (base/remote).parent.iterdir()], ['progress.log'])


if __name__ == '__main__':
    unittest.main()
