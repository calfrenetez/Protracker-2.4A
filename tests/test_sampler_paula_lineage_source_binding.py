"""Genuine typed-reader and independent command identity; HOST acceptance only."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_sampler_paula_lineage import LINEAGE_SOURCES
ROOT=Path(__file__).resolve().parents[1]
# The fixture includes the real production layout once; omit its standalone unit.
SOURCES=['tests/sampler_paula_lineage_source_binding_test.c',*LINEAGE_SOURCES[2:]]
MARKER=('LINEAGE SOURCE BINDING PASS:198 cases;36 controls162 refusals;trigger/control/stop '
        'ready/active outputs preserved,no new dispatch,independent ticket retirement,zero owned bytes')
class LineageSourceBinding(unittest.TestCase):
    def test_genuine_trigger_control_stop_cache_identity(self):
        with tempfile.TemporaryDirectory(prefix='pt-lineage-source-binding-') as folder:
            exe=Path(folder)/'fixture'
            r=subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-UNDEBUG',
                '-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],
                cwd=ROOT,capture_output=True,check=True,timeout=120)
            self.assertEqual((r.stdout,r.stderr),(b'',b''))
            r=subprocess.run([str(exe)],capture_output=True,check=True,timeout=30)
            self.assertEqual((r.stdout,r.stderr),((MARKER+'\n').encode('ascii'),b''))
if __name__=='__main__':unittest.main()
