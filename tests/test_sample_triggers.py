from pathlib import Path
import hashlib,json,struct,sys,unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_trigger_fixtures import fixtures
from test_offset_emulator import decode_trace

def check_triggers(data,trace):
    state=[[0xffffffff,1,0xffffffff,1,0,0,0xffffffff,0,0] for _ in range(4)]
    commands=[(0,0,0)]*4;fetched=0;hits=[[] for _ in range(4)]
    def trigger(v,ch,tick):
        v[6:]=[v[0],v[1],v[8]+1];hits[ch].append(tick)
    def offset(v,param):
        if param:v[4]=param
        amount=v[4]*128
        if amount<v[1]:v[0]+=amount*2;v[1]-=amount
        else:v[1]=1
    for i in range(0,len(trace),140):
        r=trace[i:i+140];count=int.from_bytes(r[28:30],'big');fresh=count!=fetched
        tick=int.from_bytes(r[:4],'big');counter=r[10]
        for ch,v in enumerate(state):
            if fresh:
                row=int.from_bytes(r[6:8],'big')//16
                e=data[1084+(row*4+ch)*4:1088+(row*4+ch)*4]
                period=((e[0]&15)<<8)|e[1];inst=(e[0]&240)|(e[2]>>4);effect=e[2]&15;param=e[3]
                commands[ch]=(period,effect,param)
                if inst:
                    assert inst==1
                    length=int.from_bytes(data[42:44],'big');a=int.from_bytes(data[46:48],'big');b=int.from_bytes(data[48:50],'big')
                    v[:4]=[2108,a+b if a else length,2108+a*2,b]
                if period and not(effect==14 and param>>4==13):
                    if effect==9:offset(v,param)
                    trigger(v,ch,tick)
                if effect==9:offset(v,param)
            period,effect,param=commands[ch];command=param>>4;amount=param&15
            if effect==14:
                if command==9 and amount and not(counter==0 and period) and counter%amount==0:trigger(v,ch,tick)
                if command==13 and period and counter==amount:trigger(v,ch,tick)
            actual=list(struct.unpack('>IHIHBBIHH',r[52+ch*22:74+ch*22]))
            assert actual==v,(tick,ch,actual,v)
        fetched=count
    return hits

class SampleTriggers(unittest.TestCase):
    def test_native_runner_preflight_fails_closed(self):
        import test_trigger_emulator as runner
        with patch.object(runner.subprocess,'check_output',side_effect=PermissionError('inspection denied')), patch.object(Path,'write_text') as write, patch.object(runner.subprocess,'Popen') as launch:
            with self.assertRaises(PermissionError):runner.main()
            write.assert_not_called();launch.assert_not_called()

    def test_native_trigger_timing(self):
        evidence=ROOT/'evidence/enhanced-editor/dev42/native'
        report=json.loads((evidence/'native-trigger.json').read_text())
        expected={'retrig_note':[[6,9]],'retrig_empty':[[6,12,14,16]],'retrig_zero':[[6]],
                  'retrig_every':[[6,7,8,9,10,11]],'delay_note':[[9]],'delay_zero':[[6]],
                  'delay_past':[[]],'trigger_repeat':[[6,8,10,14,16],[9,15]],
                  'trigger_offset':[[6,12,15,20]],'trigger_loop':[[6,8,10,14]]}
        for name,data,meta in fixtures():
            trace=(evidence/(name+'.trace')).read_bytes();case=report['cases'][name]
            self.assertEqual((evidence/(name+'.mod')).read_bytes(),data)
            self.assertEqual(hashlib.sha256(data).hexdigest(),case['fixture_sha256'])
            self.assertEqual(hashlib.sha256(trace).hexdigest(),case['trace_sha256'])
            for repeat in range(2):self.assertEqual(decode_trace((evidence/f'{name}{repeat}.log').read_text(),meta['max_ticks']),(trace,'native-stop'))
            hits=check_triggers(data,trace)
            self.assertEqual(hits[:len(expected[name])],expected[name],name)
        print('TRIGGERS: ten repeated native fixtures verify E9x/EDx counter timing, retained notes, pattern delay and offset/loop ranges; renderer enablement remains open')
