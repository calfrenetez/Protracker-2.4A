"""Genuine remaining420 cases, partitioned into explicit source/backend families."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_sampler_mixed_source_binding import SOURCES as MIXED_SOURCES

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/native_mixed_binding_format_family_test.c', *MIXED_SOURCES[1:]]
FAMILIES = {'paula': 0, 'amigus8be': 1, 'amigus8le': 2, 'amigus16be': 3, 'amigus16le': 4}
FORMATS = {'8stereo': (8, 2), '16mono': (16, 1), '16stereo': (16, 2),
           '24mono': (24, 1), '24stereo': (24, 2)}


def format_family_markers(component, name):
    bits, channels = FORMATS[component]
    assert name in FAMILIES
    faults = 6 if channels == 2 else 5
    count = faults * 3
    markers = ['MIXED BINDING TRACE:format-family-entered:'+str(bits)+':'+str(channels)+':0']
    for fault in range(faults):
        for phase in range(3):
            markers += ['MIXED BINDING TRACE:format-family-case-start:'+str(bits)+':'+str(phase)+':'+str(fault),
                        'MIXED BINDING TRACE:format-family-case-closed:'+str(bits)+':'+str(phase)+':'+str(fault)]
    markers += ['MIXED FORMAT FAMILY PASS:'+component+' '+name+';'+str(count)+
                ' cases;3 controls'+str(count-3)+' refusals;before-action/enqueue/active readiness;'
                'genuine pins and independent retirement;other'+str(495-count)+
                ' and legacy constructor excluded;SOFTWARE_ONLY',
                'MIXED BINDING TRACE:format-family-finished:'+str(bits)+':'+str(channels)+':'+str(count)]
    return markers


class MixedBindingFormatFamilies(unittest.TestCase):
    def check_format(self, component):
        bits, channels = FORMATS[component]
        for name, selection in FAMILIES.items():
            with self.subTest(component=component, family=name), tempfile.TemporaryDirectory(prefix='pt-mixed-format-family-') as folder:
                base = Path(folder)
                exe = base/'fixture'
                remote = 'RAM:PT24G-'+'ab'*8+'/progress.log'
                (base/remote).parent.mkdir()
                r = subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                    '-fsanitize=address,undefined', '-Isrc/core', '-I.',
                    '-DPT_MIXED_BINDING_BITS='+str(bits), '-DPT_MIXED_BINDING_CHANNELS='+str(channels),
                    '-DPT_MIXED_FAMILY='+str(selection), *SOURCES, '-o', str(exe)],
                    cwd=ROOT, capture_output=True, timeout=120)
                self.assertEqual((r.returncode, r.stdout, r.stderr), (0, b'', b''),
                                 (r.stdout+r.stderr).decode('utf-8', 'replace'))
                r = subprocess.run([str(exe), remote], cwd=base, capture_output=True, timeout=30)
                self.assertEqual(r.returncode, 0, (r.stdout+r.stderr).decode('utf-8', 'replace'))
                self.assertEqual(r.stderr, b'')
                markers = format_family_markers(component, name)
                self.assertEqual(len(markers), 39 if channels == 2 else 33)
                self.assertEqual(r.stdout.decode('ascii').splitlines(), markers)
                self.assertEqual((base/remote).read_bytes(), (markers[-1]+'\n').encode('ascii'))
                self.assertEqual([p.name for p in (base/remote).parent.iterdir()], ['progress.log'])

    def test_8stereo(self):
        self.check_format('8stereo')

    def test_16mono(self):
        self.check_format('16mono')

    def test_16stereo(self):
        self.check_format('16stereo')

    def test_24mono(self):
        self.check_format('24mono')

    def test_24stereo(self):
        self.check_format('24stereo')


if __name__ == '__main__':
    unittest.main()
