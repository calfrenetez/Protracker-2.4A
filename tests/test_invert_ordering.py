from pathlib import Path
import hashlib,json
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from shared_infra_invert import canonical_trace,canonical_handoff
from make_invert_fixtures import handoff_fixtures
from test_invert_emulator import decode_trace

class InvertOrdering(unittest.TestCase):
    def test_relocation_preserves_semantics(self):
        def capture(base):
            trace=bytearray(328)
            for tick in range(2):
                for ch in range(4):
                    for field in (0,6,14):
                        pos=tick*164+52+22*ch+field
                        value=base+(256 if field==6 else 0) if ch<2 else 0xffffffff
                        trace[pos:pos+4]=value.to_bytes(4,'big')
                trace[tick*164+140:tick*164+144]=(base+257+tick).to_bytes(4,'big')
                trace[tick*164+148:tick*164+164]=bytes(range(16))
            return trace
        a=capture(2108);b=capture(0xffff1234)
        self.assertEqual(canonical_trace(a),canonical_trace(b))
        for offset in (36,58,66,80,140,144,145,148,163,304):
            bad=bytearray(b);bad[offset]^=1
            self.assertNotEqual(canonical_trace(a),canonical_trace(bad),offset)
        bad=bytearray(b);bad[216]^=1
        with self.assertRaises(AssertionError):canonical_trace(bad)

    def test_handoff_relocation_preserves_semantics(self):
        data=next(handoff_fixtures())[1]
        def capture(bases):
            trace=bytearray(3*164)
            for tick,inst in enumerate((0,1,0)):
                r=tick*164
                trace[r+6:r+8]=(tick*16).to_bytes(2,'big')
                for ch in range(4):
                    for field in (0,6,14):
                        pos=r+52+22*ch+field
                        value=(bases[inst]+(64 if inst else 256))&0xffffffff if field==6 else bases[inst]
                        trace[pos:pos+4]=(value if ch==0 else 0xffffffff).to_bytes(4,'big')
                trace[r+140:r+144]=((bases[inst]+(65 if inst else 257))&0xffffffff).to_bytes(4,'big')
            return trace
        a=capture((2108,4156));b=capture((0xffff5000,0x12340))
        self.assertEqual(canonical_handoff(a,data),canonical_handoff(b,data))
        for offset in (36,61,69,143,144,145,148,163,307):
            bad=bytearray(b);bad[offset]^=1
            self.assertNotEqual(canonical_handoff(a,data),canonical_handoff(bad,data),offset)
        bad=bytearray(b);bad[2*164+52]^=1
        with self.assertRaises(AssertionError):canonical_handoff(bad,data)

    def test_handoff_fixture_and_capture_identity(self):
        evidence=ROOT/'evidence/enhanced-editor/invert-handoff'
        report=json.loads((evidence/'result.json').read_text())
        self.assertTrue(report['passed'] and report['owned_files_cleaned'])
        for name,data,meta in handoff_fixtures():
            self.assertEqual((evidence/(name+'.mod')).read_bytes(),data)
            self.assertEqual(hashlib.sha256(data).hexdigest(),report['cases'][name]['fixture_sha256'])
            normalized=(evidence/(name+'.trace')).read_bytes()
            for repeat in range(2):
                trace,reason=decode_trace((evidence/(name+str(repeat)+'.log')).read_text(),meta['max_ticks'])
                self.assertEqual(reason,'native-stop')
                self.assertEqual(canonical_handoff(trace,data),normalized)
                if not repeat:self.assertEqual(hashlib.sha256(trace).hexdigest(),report['cases'][name]['trace_sha256'])

    def test_flow_matches_pinned_native_mutations(self):
        evidence=ROOT/'evidence/enhanced-editor/invert-ordering'
        cases=list(evidence.glob('*.mod'))
        self.assertEqual(len(cases),3)
        one_shots=list((ROOT/'evidence/enhanced-editor/invert-oneshot').glob('*.mod'))
        self.assertEqual(len(one_shots),3)
        handoffs=list((ROOT/'evidence/enhanced-editor/invert-handoff').glob('*.mod'))
        self.assertEqual(len(handoffs),4)
        cases+=one_shots+handoffs
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            sources=['invert_bank','invert_sequence','invert_pcm','invert_loop','flow','document','pp20','safe_save','mod_project','mod_inspect','project','channels','pcm']
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/invert_ordering_test.c',*[f'src/core/{s}.c' for s in sources],'-o',str(exe)],cwd=ROOT,check=True)
            for mod in sorted(cases):
                for repeat in range(2):
                    subprocess.run([str(exe),str(mod),str(mod.with_name(mod.stem+str(repeat)+'.log'))],check=True)
