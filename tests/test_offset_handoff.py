from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,unittest
from test_instrument_render import SOURCES
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_handoff_fixtures import offset_handoff_fixtures
from test_instrument_emulator import decode_trace
class OffsetHandoff(unittest.TestCase):
    def test_pinned_volume_and_pcm(self):
        evidence=ROOT/'evidence/enhanced-editor/dev85/native-reference'
        report=json.loads((evidence/'native-instrument.json').read_text())
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/render_handoff_test.c',*SOURCES,'src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c','-o',str(exe)],cwd=ROOT,check=True)
            for name,data,meta in offset_handoff_fixtures():
                trace=(evidence/(name+'.trace')).read_bytes();case=report['cases'][name]
                self.assertEqual((evidence/(name+'.mod')).read_bytes(),data)
                self.assertEqual(hashlib.sha256(data).hexdigest(),case['fixture_sha256'])
                self.assertEqual(hashlib.sha256(trace).hexdigest(),case['trace_sha256'])
                for repeat in range(2):self.assertEqual(decode_trace((evidence/(name+str(repeat)+'.log')).read_text(),100),(trace,'native-stop'))
                seen=set();row_counts={}
                for offset in range(0,len(trace),140):
                    r=trace[offset:offset+140]
                    if not r[14] or not int.from_bytes(r[28:30],'big'):continue
                    row=int.from_bytes(r[6:8],'big')//16;seen.add(row)
                    row_counts[row]=row_counts.get(row,0)+1
                    self.assertEqual(int.from_bytes(r[12:14],'big'),125)
                    self.assertEqual(r[32],64 if row==0 else 32)
                    stored_start,stored_words=(2620,256) if row==0 else ((4156,160) if name=='handoff_offsetload' else (4412,32))
                    self.assertEqual(int.from_bytes(r[52:56],'big'),stored_start)
                    self.assertEqual(int.from_bytes(r[56:58],'big'),stored_words)
                    self.assertEqual(int.from_bytes(r[58:62],'big'),2364 if row==0 else 4220)
                    self.assertEqual(int.from_bytes(r[62:64],'big'),384 if row==0 else 128)
                    self.assertEqual(int.from_bytes(r[72:74],'big'),1 if row<2 else (2 if r[10]>=3 else 1) if meta['delay'] else 2+r[10]//2)
                self.assertEqual(seen,{0,1,2})
                self.assertEqual(row_counts[1],6)
                subprocess.run([str(exe),str(evidence/(name+'.mod')),str(evidence/(name+'0.log')),'1'],check=True)
