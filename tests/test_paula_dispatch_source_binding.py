"""Prepared Paula trigger source identity, genuine ownership and public apply."""
from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES as SAMPLER
from test_paula_preflight import SOURCES as PREFLIGHT
ROOT=Path(__file__).resolve().parents[1]
SOURCES=list(dict.fromkeys(['tests/paula_dispatch_source_binding_test.c','src/editor/paula_dispatch.c',
    'src/editor/paula_voices.c','src/editor/sampler_paula.c','src/core/sample_cache.c',
    'src/core/playback_pcm.c',*SAMPLER[1:],*PREFLIGHT[1:]]))
MARKER=('PAULA DISPATCH SOURCE BINDING PASS:60 cases;12 controls48 source/representation refusals;'
        'final preparation,ready/private/public apply;masters preserved,no refused output,genuine cleanup')
class DispatchBinding(unittest.TestCase):
    def test_genuine_prepared_trigger_keys_and_apply(self):
        with tempfile.TemporaryDirectory(prefix='pt-dispatch-binding-') as folder:
            exe=Path(folder)/'fixture'
            r=subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-UNDEBUG',
                '-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],
                cwd=ROOT,capture_output=True,timeout=120)
            self.assertEqual(r.returncode,0,(r.stdout+r.stderr).decode('utf8','replace'))
            self.assertEqual((r.stdout,r.stderr),(b'',b''))
            r=subprocess.run([str(exe)],capture_output=True,timeout=30)
            self.assertEqual(r.returncode,0,(r.stdout+r.stderr).decode('utf8','replace'))
            self.assertEqual((r.stdout,r.stderr),((MARKER+'\n').encode('ascii'),b''))
if __name__=='__main__':unittest.main()
