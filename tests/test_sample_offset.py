from pathlib import Path
import hashlib,json,struct,sys,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_offset_fixtures import fixtures
from test_offset_emulator import decode_trace
from test_porta_emulator import decode_trace as decode_pitch

def check_ranges(data,trace):
    # Model the pinned classic word-length instruction sequence independently
    # of the assembly instrumentation. Pointer values are relative to MOD base.
    state=[[0xffffffff,1,0xffffffff,1,0,0,0xffffffff,0,0] for _ in range(4)]
    fetched=0
    def apply(v,param):
        if param:v[4]=param
        amount=v[4]*128
        if amount<v[1]:v[0]+=amount*2;v[1]-=amount
        else:v[1]=1
    for i in range(0,len(trace),140):
        r=trace[i:i+140];count=int.from_bytes(r[28:30],'big')
        if count!=fetched:
            row=int.from_bytes(r[6:8],'big')//16
            for ch,v in enumerate(state):
                e=data[1084+(row*4+ch)*4:1088+(row*4+ch)*4];period=((e[0]&15)<<8)|e[1];inst=(e[0]&240)|(e[2]>>4);effect=e[2]&15;param=e[3]
                if inst:
                    assert inst==1
                    length=int.from_bytes(data[42:44],'big');a=int.from_bytes(data[46:48],'big');b=int.from_bytes(data[48:50],'big')
                    v[:4]=[2108,a+b if a else length,2108+a*2,b]
                if period:
                    assert effect not in (3,5) and not(effect==14 and param>>4==13)
                    if effect==9:apply(v,param)
                    v[6:]=[v[0],v[1],v[8]+1]
                if effect==9:apply(v,param)
            fetched=count
        for ch,v in enumerate(state):
            actual=list(struct.unpack('>IHIHBBIHH',r[52+ch*22:74+ch*22]))
            assert actual==v,(i//140+1,ch,actual,v)
    return state

class SampleOffset(unittest.TestCase):
    def test_native_sample_ranges_and_triggers(self):
        evidence=ROOT/'evidence/enhanced-editor/dev39/native';report=json.loads((evidence/'native-offset.json').read_text())
        baseline,_=decode_trace((evidence/'baseline.log').read_text(),160)
        old,_=decode_pitch((ROOT/'evidence/enhanced-editor/dev38/native/baseline.log').read_text(),160)
        self.assertEqual(b''.join(baseline[i:i+52] for i in range(0,len(baseline),140)),old)
        for name,data,meta in fixtures():
            trace=(evidence/(name+'.trace')).read_bytes();case=report['cases'][name]
            self.assertEqual((evidence/(name+'.mod')).read_bytes(),data)
            self.assertEqual(hashlib.sha256(data).hexdigest(),case['fixture_sha256'])
            self.assertEqual(hashlib.sha256(trace).hexdigest(),case['trace_sha256'])
            for repeat in range(2):self.assertEqual(decode_trace((evidence/f'{name}{repeat}.log').read_text(),meta['max_ticks']),(trace,'native-stop'))
            state=check_ranges(data,trace)
            if name=='offset_double':self.assertEqual(state[0],[2620,768,2108,1,1,0,2364,896,1])
        print('OFFSET: eight repeated native traces verify stored ranges, initial trigger ranges, loops, memory, bounds, delayed rows and four independent tracks; shipping 9xx still refused')
