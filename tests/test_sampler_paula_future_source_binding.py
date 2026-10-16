"""Genuine independently retained trigger/cache identity; HOST acceptance only."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_sampler_paula_future import FUTURE_SOURCES
ROOT=Path(__file__).resolve().parents[1]
# The new fixture includes production future.c once, with the real layout.
SOURCES=['tests/sampler_paula_future_source_binding_test.c',*FUTURE_SOURCES[2:]]
MARKER=('FUTURE SOURCE BINDING PASS:66 cases;12 controls54 refusals;ready/live outputs '
        'preserved,no dispatch,pins retained until independent retirement,zero owned bytes')
class FutureSourceBinding(unittest.TestCase):
    def test_genuine_ready_and_live_cache_identity(self):
        with tempfile.TemporaryDirectory(prefix='pt-future-source-binding-') as folder:
            exe=Path(folder)/'fixture'
            r=subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-UNDEBUG',
                '-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],
                cwd=ROOT,capture_output=True,check=True,timeout=120)
            self.assertEqual((r.stdout,r.stderr),(b'',b''))
            r=subprocess.run([str(exe)],capture_output=True,check=True,timeout=30)
            self.assertEqual((r.stdout,r.stderr),((MARKER+'\n').encode('ascii'),b''))
if __name__=='__main__':unittest.main()
