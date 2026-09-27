from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_invert_fixtures import shared_handoff_fixtures
from shared_infra_invert import canonical_commands
from test_invert_emulator import decode_trace
from test_invert_handoff_workflows import WORKFLOW_SOURCES,verify_outputs
CORE=['render_invert','render','invert_bank','invert_sequence','invert_pcm','invert_loop','pitch','timeline','frame_clock','flow','voice','document','pp20','safe_save','mod_project','mod_inspect','project','channels','pcm']
EVIDENCE=ROOT/'evidence/enhanced-editor/invert-shared-handoff'
def compile_test(exe,source,sources,flags=()):
    subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*flags,source,*sources,'-o',str(exe)],cwd=ROOT,check=True)
def cases():
    result=sorted(EVIDENCE.glob('*.mod'));assert len(result)==3;return result
class InvertSharedHandoff(unittest.TestCase):
    def test_reference_identity_and_two_clocks(self):
        report=json.loads((EVIDENCE/'result.json').read_text())
        self.assertTrue(report['passed'] and report['owned_files_cleaned'])
        for name,data,meta in shared_handoff_fixtures():
            self.assertEqual((EVIDENCE/(name+'.mod')).read_bytes(),data)
            self.assertEqual(hashlib.sha256(data).hexdigest(),report['cases'][name]['fixture_sha256'])
            normalized=(EVIDENCE/(name+'.trace')).read_bytes()
            for repeat in range(2):
                raw,reason=decode_trace((EVIDENCE/(name+str(repeat)+'.log')).read_text(),100,188)
                self.assertEqual(reason,'native-stop');self.assertEqual(canonical_commands(raw,data,188),normalized)
                if not repeat:self.assertEqual(hashlib.sha256(raw).hexdigest(),report['cases'][name]['trace_sha256'])
            # Neither the second clock nor its independently bound loop bytes
            # disappear during relocation normalization.
            for field in (167,168,169,172,187):
                bad=bytearray(raw);bad[12*188+field]^=1
                self.assertNotEqual(canonical_commands(bad,data,188),normalized)
            records=[normalized[n:n+188] for n in range(0,len(normalized),188)]
            self.assertTrue(any(r[144:146]!=r[168:170] for r in records))
            for r in records:
                if r[58:62]==r[80:84] and r[58:62]!=b'\xff'*4:
                    size=int.from_bytes(r[62:64],'big')*2
                    self.assertEqual(r[148:148+size],r[172:172+size])
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'ordering'
            compile_test(exe,'tests/invert_ordering_test.c',[f'src/core/{s}.c' for s in CORE])
            for mod in cases():
                for repeat in range(2):subprocess.run([str(exe),str(mod),str(mod.with_name(mod.stem+str(repeat)+'.log'))],check=True)

    def test_reference_pcm_and_audibility(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'pcm'
            compile_test(exe,'tests/render_invert_handoff_test.c',[f'src/core/{s}.c' for s in CORE],['-DINVERT_RECORD_BYTES=188'])
            for mod in cases():subprocess.run([str(exe),str(mod),str(mod.with_suffix('.trace'))],check=True)

    def test_exports_and_master_transactions(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);exe=tmp/'workflow'
            compile_test(exe,'tests/invert_handoff_workflow_test.c',WORKFLOW_SOURCES)
            for mod in cases():
                out=tmp/mod.stem;out.mkdir()
                subprocess.run([str(exe),str(mod),str(out)],check=True)
                verify_outputs(out,mod.read_bytes(),mod.with_suffix('.trace').read_bytes(),188)
