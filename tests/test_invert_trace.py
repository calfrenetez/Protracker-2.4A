from pathlib import Path
import hashlib,json,sys,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_invert_fixtures import fixtures
from test_invert_emulator import decode_trace
class InvertTrace(unittest.TestCase):
    def test_pinned_byte_mutations(self):
        base=ROOT/'evidence/enhanced-editor/dev91/native-reference'
        report=json.loads((base/'native-invert.json').read_text())
        for name,data,meta in fixtures():
            trace=(base/(name+'.trace')).read_bytes()
            self.assertEqual((base/(name+'.mod')).read_bytes(),data)
            self.assertEqual(hashlib.sha256(data).hexdigest(),report['cases'][name]['fixture_sha256'])
            self.assertEqual(hashlib.sha256(trace).hexdigest(),report['cases'][name]['trace_sha256'])
            for repeat in range(2):self.assertEqual(decode_trace((base/(name+str(repeat)+'.log')).read_text(),100),(trace,'native-stop'))
            pcm=list(range(16));cursor=accumulator=0;speed=meta['speed'];seen=set()
            for offset in range(0,len(trace),164):
                r=trace[offset:offset+164]
                if not r[14] or not int.from_bytes(r[28:30],'big'):continue
                row=int.from_bytes(r[6:8],'big')//16;tick=r[10];seen.add(row)
                if row==1 and tick==0 and meta['disable']:speed=0
                if speed and (tick or row==0):
                    accumulator+={8:16,15:128}[speed]
                    if accumulator>=128:
                        accumulator=0;cursor=(cursor+1)%16;pcm[cursor]^=255
                self.assertEqual(int.from_bytes(r[140:144],'big'),2364+cursor)
                self.assertEqual(r[144]>>4,speed)
                self.assertEqual(r[145],accumulator)
                self.assertEqual(r[148:164],bytes(pcm))
            self.assertEqual(seen,{0,1,2})
