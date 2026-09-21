from pathlib import Path
import hashlib,json,sys,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_handoff_fixtures import fixtures
from test_instrument_emulator import decode_trace
class HandoffTrace(unittest.TestCase):
    def test_pinned_stored_ranges_without_new_trigger(self):
        evidence=ROOT/'evidence/enhanced-editor/dev66/native'
        report=json.loads((evidence/'native-instrument.json').read_text())
        word=lambda r,n:int.from_bytes(r[n:n+2],'big')
        long=lambda r,n:int.from_bytes(r[n:n+4],'big')
        for name,data,meta in fixtures():
            case=report['cases'][name];trace=(evidence/(name+'.trace')).read_bytes()
            self.assertEqual((evidence/(name+'.mod')).read_bytes(),data)
            self.assertEqual(hashlib.sha256(data).hexdigest(),case['fixture_sha256'])
            self.assertEqual(hashlib.sha256(trace).hexdigest(),case['trace_sha256'])
            for repeat in range(2):self.assertEqual(decode_trace((evidence/(name+str(repeat)+'.log')).read_text(),100),(trace,'native-stop'))
            seen=set()
            for pos in range(0,len(trace),140):
                r=trace[pos:pos+140]
                if not r[14] or not word(r,28):continue
                row=word(r,6)//16;seen.add(row)
                sample=next(inst for change,inst in reversed(meta['changes']) if change<=row)
                start=2108 if sample==1 else 4156
                loop=start+(256 if sample==1 else 64 if meta['second_loop'] else 0)
                length=512 if sample==1 else 160 if meta['second_loop'] else 256
                repeat=384 if sample==1 else 128 if meta['second_loop'] else 1
                self.assertEqual((long(r,52),word(r,56),long(r,58),word(r,62)),(start,length,loop,repeat))
                self.assertEqual((long(r,66),word(r,70),word(r,72)),(2108,512,1))
                self.assertEqual(r[32],64 if sample==1 else 32)
                self.assertEqual(word(r,44),428)
            self.assertEqual(seen,set(range(max(row for row,_ in meta['changes'])+1)))
