"""Run the exact finite native probe with host DOS/Exec timing/signal shims."""
from pathlib import Path
import os,subprocess,tempfile,unittest
from test_editor import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class ViewProbe(unittest.TestCase):
    def test_exact_pixels_and_cooperative_break(self):
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp);(d/'proto').mkdir();(d/'dos').mkdir()
            (d/'proto/dos.h').write_text('struct DateStamp {long ds_Days,ds_Minute,ds_Tick;};\nvoid DateStamp(struct DateStamp *);\n')
            (d/'proto/exec.h').write_text('unsigned long SetSignal(unsigned long,unsigned long);\n')
            (d/'dos/dos.h').write_text('#define SIGBREAKF_CTRL_C 4096UL\n')
            font=(ROOT/'vendor/pt23f/raw/ptfont.raw').read_bytes()
            (d/'pt_font.h').write_text('static const unsigned char pt_font[]={'+','.join(map(str,font))+'};\n')
            shim=d/'shim.c';shim.write_text('#include <proto/dos.h>\n#include <stdlib.h>\nvoid DateStamp(struct DateStamp *p){static long t; p->ds_Days=p->ds_Minute=0;p->ds_Tick=t++;}\nunsigned long SetSignal(unsigned long a,unsigned long b){static unsigned n;(void)a;(void)b;return getenv("PT_PROBE_BREAK") && ++n==26 ? 4096UL:0;}\n')
            binary=d/'probe'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','-I'+str(d),'tests/native_view_probe.c',str(shim),*SOURCES[1:],'-o',str(binary)],cwd=ROOT,check=True)
            passed=subprocess.run([str(binary)],text=True,capture_output=True,check=True)
            self.assertIn('identical=1',passed.stdout);self.assertIn('PT24G VIEW PASS',passed.stdout)
            broken=subprocess.run([str(binary)],text=True,capture_output=True,env={**os.environ,'PT_PROBE_BREAK':'1'})
            self.assertEqual(broken.returncode,20);self.assertEqual(broken.stdout,'PT24G VIEW FAIL\n');self.assertEqual(broken.stderr,'')
