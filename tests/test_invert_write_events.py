from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_invert_fixtures import long_loop_fixtures
from shared_infra_invert import canonical_commands
from test_invert_emulator import decode_trace
from test_invert_shared_handoff import CORE,compile_test
from test_invert_handoff_workflows import WORKFLOW_SOURCES,verify_outputs
EVIDENCE=ROOT/'evidence/enhanced-editor/invert-write-events'
def records(trace):return [trace[i:i+208] for i in range(0,len(trace),208)]
def events(r):
    n=int.from_bytes(r[140:142],'big');assert n<=8 and r[142:144]==bytes(2)
    assert r[144+8*n:]==bytes(64-8*n)
    return [(int.from_bytes(r[144+i*8:148+i*8],'big'),r[148+i*8],r[149+i*8]) for i in range(n)]
def cases():
    result=sorted(EVIDENCE.glob('*.mod'));assert len(result)==4;return result
class InvertWrites(unittest.TestCase):
    def test_identity_neutrality_and_event_coverage(self):
        report=json.loads((EVIDENCE/'result.json').read_text());self.assertTrue(report['passed'] and report['owned_files_cleaned'])
        for name,data,meta in long_loop_fixtures():
            self.assertEqual((EVIDENCE/(name+'.mod')).read_bytes(),data)
            self.assertEqual(hashlib.sha256(data).hexdigest(),report['cases'][name]['fixture_sha256'])
            normalized=(EVIDENCE/(name+'.trace')).read_bytes()
            for repeat in range(2):
                raw,reason=decode_trace((EVIDENCE/(name+str(repeat)+'.log')).read_text(),100,208)
                self.assertEqual(reason,'native-stop');self.assertEqual(canonical_commands(raw,data,208),normalized)
                if not repeat:self.assertEqual(hashlib.sha256(raw).hexdigest(),report['cases'][name]['trace_sha256'])
            seen=[];bank=bytearray(data);collision=False
            old=(ROOT/'evidence/enhanced-editor/invert-shared-handoff/inv_shared_split.trace').read_bytes() if meta['loop_bytes']==16 else None
            for tick,r in enumerate(records(normalized)):
                writes=events(r);addresses=[e[0] for e in writes];collision|=len(set(addresses))<len(addresses)
                for address,value,mask in writes:
                    self.assertIn(mask,(1,2));self.assertGreaterEqual(address,2108);self.assertLess(address,len(data))
                    self.assertEqual(bank[address]^255,value);bank[address]=value;seen.append((address,mask))
                if old is not None:
                    prior=old[tick*188:(tick+1)*188];self.assertEqual(r[:140],prior[:140])
                    for ch in (0,1):
                        begin=int.from_bytes(prior[58+22*ch:62+22*ch],'big');size=int.from_bytes(prior[62+22*ch:64+22*ch],'big')*2
                        if begin!=0xffffffff:self.assertEqual(bank[begin:begin+size],prior[148+24*ch:148+24*ch+size])
            if meta['loop_bytes']>16:self.assertTrue(collision,name)
            if meta['loop_bytes']>16:self.assertTrue(any(address>=2364+16 and address<2364+meta['loop_bytes'] for address,mask in seen))
            if meta['loop_bytes']==18:self.assertIn((2364,1),seen) # wrap to loop start
            first=next(i for i,r in enumerate(records(raw)) if events(r))
            for field in (147,148,149):
                bad=bytearray(raw);bad[first*208+field]^=1
                self.assertNotEqual(canonical_commands(bad,data,208),normalized)
            for field,value in ((141,9),(143,1),(207,1)):
                bad=bytearray(raw);bad[first*208+field]=value
                with self.assertRaises(AssertionError):canonical_commands(bad,data,208)

    def test_full_bank_mutations_each_tick(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);exe=tmp/'ordering'
            compile_test(exe,'tests/invert_ordering_test.c',[f'src/core/{s}.c' for s in CORE])
            for mod in cases():
                trace=mod.with_suffix('.trace').read_bytes();rs=records(trace)
                log=tmp/(mod.stem+'.log');log.write_text('FLOW schema=1 bytes=208 count='+str(len(rs))+' reason=native-stop\n'+''.join('T '+r.hex()+'\n' for r in rs)+'FLOW PASS dma=0\n')
                subprocess.run([str(exe),str(mod),str(log)],check=True)

    def test_reference_pcm_offline_incremental_and_mutes(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'pcm'
            compile_test(exe,'tests/render_invert_handoff_test.c',[f'src/core/{s}.c' for s in CORE],['-DINVERT_RECORD_BYTES=208'])
            for mod in cases():subprocess.run([str(exe),str(mod),str(mod.with_suffix('.trace'))],check=True)

    def test_exports_bounce_and_master_transactions(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);exe=tmp/'workflow'
            compile_test(exe,'tests/invert_handoff_workflow_test.c',WORKFLOW_SOURCES)
            for mod in cases():
                out=tmp/mod.stem;out.mkdir()
                frames=17280 if mod.stem.endswith('baseline') else 40320
                subprocess.run([str(exe),str(mod),str(out),str(frames)],check=True)
                verify_outputs(out,mod.read_bytes(),mod.with_suffix('.trace').read_bytes(),208)
