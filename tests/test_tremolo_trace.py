from pathlib import Path
import hashlib,json,sys,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_tremolo_fixtures import fixtures
from test_tremolo_emulator import decode_trace
SINE=[0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,253,250,244,235,224,212,197,180,161,141,120,97,74,49,24]
def check_tremolo(data,trace):
    state=[[0,0,0,0] for _ in range(4)];base=[0]*4;vibcmd=[0]*4;commands=[(0,0)]*4;fetches=0;different=0
    for at in range(0,len(trace),76):
        r=trace[at:at+76];f=int.from_bytes(r[28:30],'big');fresh=f!=fetches
        for ch,v in enumerate(state):
            if fresh:
                row=int.from_bytes(r[6:8],'big')//16;e=data[1084+(row*4+ch)*4:1088+(row*4+ch)*4]
                period=((e[0]&15)<<8)|e[1];inst=(e[0]&240)|(e[2]>>4);effect=e[2]&15;param=e[3];commands[ch]=(effect,param)
                if inst:base[ch]=data[45]
                if period and effect not in (3,5) and not(effect==14 and param>>4==13):
                    if not v[2]&4:v[3]=0
                    if not v[2]&64:v[1]=0
            effect,param=commands[ch];out=base[ch]
            if effect==14 and param>>4==7:v[2]=(v[2]&15)|((param&15)<<4)
            if not fresh and effect==4:
                if param&15:vibcmd[ch]=(vibcmd[ch]&240)|(param&15)
                if param&240:vibcmd[ch]=(vibcmd[ch]&15)|(param&240)
                v[3]=(v[3]+(vibcmd[ch]>>4)*4)&255
            if not fresh and effect==7:
                if param&15:v[0]=(v[0]&240)|(param&15)
                if param&240:v[0]=(v[0]&15)|(param&240)
                index=(v[1]>>2)&31;wave=(v[2]>>4)&3
                amp=SINE[index] if wave==0 else (255-index*8 if v[3]&128 else index*8) if wave==1 else 255
                delta=amp*(v[0]&15)//64
                out=max(0,min(64,base[ch]+(-delta if v[1]&128 else delta)))
                v[1]=(v[1]+(v[0]>>4)*4)&255
            assert int.from_bytes(r[52+ch*2:54+ch*2],'big')==base[ch],(at//76,ch,'stored volume')
            assert list(r[60+4*ch:64+4*ch])==v,(at//76,ch,list(r[60+4*ch:64+4*ch]),v)
            assert r[32+ch]==out,(at//76,ch,r[32+ch],out)
            different+=out!=base[ch]
        fetches=f
    return different

class TremoloTrace(unittest.TestCase):
    def test_native_volume_and_memory(self):
        evidence=ROOT/'evidence/enhanced-editor/dev45/native';report=json.loads((evidence/'native-tremolo.json').read_text())
        for name,data,meta in fixtures():
            trace=(evidence/(name+'.trace')).read_bytes();case=report['cases'][name]
            self.assertEqual((evidence/(name+'.mod')).read_bytes(),data)
            self.assertEqual(hashlib.sha256(data).hexdigest(),case['fixture_sha256'])
            self.assertEqual(hashlib.sha256(trace).hexdigest(),case['trace_sha256'])
            for repeat in range(2):self.assertEqual(decode_trace((evidence/f'{name}{repeat}.log').read_text(),meta['max_ticks']),(trace,'native-stop'))
            self.assertGreater(check_tremolo(data,trace),0,name)
        print('TREMOLO: eight repeated native traces verify output/stored volume, waveforms, nibble memory, reset control, delay and vibrato-phase ramp dependency; renderer support remains open')
