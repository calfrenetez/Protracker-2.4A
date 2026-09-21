from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,unittest
from test_instrument_render import SOURCES
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_handoff_fixtures import cut_fixtures
from test_instrument_emulator import decode_trace
class CutHandoff(unittest.TestCase):
    def test_pinned_volume_and_pcm(self):
        evidence=ROOT/'evidence/enhanced-editor/dev71/native-reference'
        report=json.loads((evidence/'native-instrument.json').read_text())
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/render_handoff_test.c',*SOURCES,'src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c','-o',str(exe)],cwd=ROOT,check=True)
            for name,data,meta in cut_fixtures():
                trace=(evidence/(name+'.trace')).read_bytes();case=report['cases'][name]
                self.assertEqual((evidence/(name+'.mod')).read_bytes(),data)
                self.assertEqual(hashlib.sha256(data).hexdigest(),case['fixture_sha256'])
                self.assertEqual(hashlib.sha256(trace).hexdigest(),case['trace_sha256'])
                for repeat in range(2):self.assertEqual(decode_trace((evidence/(name+str(repeat)+'.log')).read_text(),100),(trace,'native-stop'))
                seen=set()
                for offset in range(0,len(trace),140):
                    r=trace[offset:offset+140]
                    if not r[14] or not int.from_bytes(r[28:30],'big'):continue
                    row=int.from_bytes(r[6:8],'big')//16;seen.add(row)
                    counter=r[10]
                    volume=64 if row!=1 else 0 if counter>=meta['cut'] else 32
                    self.assertEqual(r[32],volume)
                    self.assertEqual(int.from_bytes(r[72:74],'big'),1)
                self.assertEqual(seen,{0,1,2})
                subprocess.run([str(exe),str(evidence/(name+'.mod')),str(evidence/(name+'0.log')),'1'],check=True)
