"""Actual active source identity and unchanged retired-reader ownership."""
from pathlib import Path
import subprocess,tempfile,unittest
from test_paula_dispatch_source_binding import SOURCES as DISPATCH
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/paula_active_source_binding_test.c',*DISPATCH[1:]]
MARKER=('PAULA ACTIVE SOURCE BINDING PASS:147 cases;27 controls120 refusals;direct/public/incremental starts,'
        'original8/16/24 mono/stereo sources,invalidated pinned copies,confirmed stop-only release')
class ActiveSourceBinding(unittest.TestCase):
    def test_original_source_across_edits_and_confirmed_stop(self):
        with tempfile.TemporaryDirectory(prefix='pt-active-binding-') as folder:
            exe=Path(folder)/'fixture'
            r=subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-UNDEBUG',
                '-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],cwd=ROOT,capture_output=True,timeout=120)
            self.assertEqual(r.returncode,0,(r.stdout+r.stderr).decode('utf8','replace'))
            self.assertEqual((r.stdout,r.stderr),(b'',b''))
            r=subprocess.run([str(exe)],capture_output=True,timeout=30)
            self.assertEqual(r.returncode,0,(r.stdout+r.stderr).decode('utf8','replace'))
            self.assertEqual((r.stdout,r.stderr),((MARKER+'\n').encode('ascii'),b''))
if __name__=='__main__':unittest.main()
