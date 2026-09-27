from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_invert_fixtures import four_clock_delay_fixtures
from shared_infra_invert import canonical_commands
from test_invert_emulator import decode_trace
from test_invert_shared_handoff import CORE,compile_test
from test_invert_handoff_workflows import WORKFLOW_SOURCES,verify_outputs
from test_invert_write_events import records,events
EVIDENCE=ROOT/'evidence/enhanced-editor/invert-four-clock-delay'
def cases():
    for name,data,meta in four_clock_delay_fixtures():yield EVIDENCE/(name+'.mod'),meta
class InvertFourClockDelay(unittest.TestCase):
    def test_reference_delayed_tick_zero_and_four_clocks(self):
        report=json.loads((EVIDENCE/'result.json').read_text())
        self.assertTrue(report['passed'] and report['owned_files_cleaned'])
        for name,data,meta in four_clock_delay_fixtures():
            self.assertEqual((EVIDENCE/(name+'.mod')).read_bytes(),data)
            self.assertEqual(hashlib.sha256(data).hexdigest(),report['cases'][name]['fixture_sha256'])
            trace=(EVIDENCE/(name+'.trace')).read_bytes()
            for repeat in range(2):
                raw,reason=decode_trace((EVIDENCE/(name+str(repeat)+'.log')).read_text(),100,208)
                self.assertEqual(reason,'native-stop');self.assertEqual(canonical_commands(raw,data,208),trace)
            rs=[r for r in records(trace) if r[14] and int.from_bytes(r[28:30],'big')]
            self.assertEqual(len(rs),meta['active_ticks'])
            bank=bytearray(data);channels=set();delayed=[]
            for r in rs:
                writes=events(r);masks=[mask for address,value,mask in writes]
                self.assertEqual(masks,sorted(masks)) # reference executes each channel in order
                for address,value,mask in writes:
                    self.assertIn(mask,(1,2,4,8));self.assertTrue(2108<=address<len(data))
                    self.assertEqual(bank[address]^255,value);bank[address]=value;channels.add(mask)
                if r[10]==0 and int.from_bytes(r[6:8],'big')==16:delayed.append(writes)
            self.assertEqual(channels,{1,2,4,8});self.assertEqual(len(delayed),meta['delay']+1)
            self.assertEqual(max(len(events(r)) for r in rs),7 if meta['delay']==2 else 5)
            for writes in delayed[1:]:
                self.assertEqual([mask for address,value,mask in writes], [1,1,2,2,4,4,8] if meta['delay']==2 else [1,1,2,2,8])
            self.assertTrue(any(len({a for a,v,m in events(r)})<len(events(r)) for r in rs))

    def test_full_private_banks_each_tick(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);exe=tmp/'ordering'
            compile_test(exe,'tests/invert_ordering_test.c',[f'src/core/{s}.c' for s in CORE])
            for mod,meta in cases():
                rs=records(mod.with_suffix('.trace').read_bytes());log=tmp/(mod.stem+'.log')
                log.write_text('FLOW schema=1 bytes=208 count='+str(len(rs))+' reason=native-stop\n'+''.join('T '+r.hex()+'\n' for r in rs)+'FLOW PASS dma=0\n')
                subprocess.run([str(exe),str(mod),str(log)],check=True)

    def test_reference_pcm_and_muted_contributors(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'pcm'
            compile_test(exe,'tests/render_invert_handoff_test.c',[f'src/core/{s}.c' for s in CORE],['-DINVERT_RECORD_BYTES=208'])
            for mod,meta in cases():subprocess.run([str(exe),str(mod),str(mod.with_suffix('.trace')),str(meta['active_ticks'])],check=True)

    def test_export_bounce_and_master_transactions(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);exe=tmp/'workflow'
            compile_test(exe,'tests/invert_handoff_workflow_test.c',WORKFLOW_SOURCES)
            for mod,meta in cases():
                out=tmp/mod.stem;out.mkdir();subprocess.run([str(exe),str(mod),str(out),str(meta['active_ticks']*960)],check=True)
                verify_outputs(out,mod.read_bytes(),mod.with_suffix('.trace').read_bytes(),208)
