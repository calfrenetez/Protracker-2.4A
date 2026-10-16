"""Genuine persistent-reader source/representation binding, without hardware."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from test_sampler_paula_readers import READERS_SOURCES

ROOT = Path(__file__).resolve().parents[1]
MARKER = ('SAMPLER READERS SOURCE BINDING PASS:66 cases;12 genuine controls,54 '
          'sample/side/representation refusals;no dispatch,master preserved,'
          'independent retirement,zero owned bytes')
SOURCES = ['tests/sampler_paula_readers_source_binding_test.c', *READERS_SOURCES[1:]]


class ReaderSourceBinding(unittest.TestCase):
    def test_native_selection_refuses_conflicting_workloads_before_inputs(self):
        for flags in ([],['--exec-memory','--trace-memory','--reader-component','lifecycle']):
            with self.subTest(flags=flags):
                result=subprocess.run([sys.executable,'-B',str(ROOT/'tools/build_paula_readers_test.py'),
                    '--source-binding',*flags,'--source-manifest','NOT_READ','--source-manifest-sha256','NOT_READ',
                    '--pins','NOT_READ','--pins-sha256','NOT_READ','--out','NOT_CREATED'],
                    cwd=ROOT,capture_output=True,timeout=10)
                self.assertEqual(result.returncode,2)
                self.assertIn(b'--source-binding requires --exec-memory without other reader components',result.stderr)
                self.assertNotIn(b'Traceback',result.stderr)

    def test_refusal_and_independent_retirement(self):
        with tempfile.TemporaryDirectory(prefix='pt-reader-source-binding-') as folder:
            executable = Path(folder)/'fixture'
            result = subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-UNDEBUG','-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(executable)],
                cwd=ROOT, check=True, capture_output=True, timeout=120)
            self.assertEqual((result.stdout,result.stderr),(b'',b''))
            result = subprocess.run([str(executable)],check=True,capture_output=True,timeout=30)
            self.assertEqual((result.stdout,result.stderr),((MARKER+'\n').encode('ascii'),b''))


if __name__ == '__main__':
    unittest.main()
