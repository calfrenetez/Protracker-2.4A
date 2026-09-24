from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from shared_infra_invert import canonical_trace

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

    def test_flow_matches_pinned_native_mutations(self):
        evidence=ROOT/'evidence/enhanced-editor/invert-ordering'
        cases=list(evidence.glob('*.mod'))
        self.assertEqual(len(cases),3)
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            sources=['invert_bank','invert_sequence','invert_pcm','invert_loop','flow','document','pp20','safe_save','mod_project','mod_inspect','project','channels','pcm']
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/invert_ordering_test.c',*[f'src/core/{s}.c' for s in sources],'-o',str(exe)],cwd=ROOT,check=True)
            for mod in sorted(cases):
                for repeat in range(2):
                    subprocess.run([str(exe),str(mod),str(mod.with_name(mod.stem+str(repeat)+'.log'))],check=True)
