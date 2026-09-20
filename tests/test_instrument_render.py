from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_instrument_fixtures import fixtures
from test_offset_emulator import decode_trace
SOURCES=['src/core/render.c','src/core/pitch.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c','src/core/voice.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
class InstrumentRender(unittest.TestCase):
    def test_reference_phase_volume_and_handoff_boundaries(self):
        evidence=ROOT/'evidence/enhanced-editor/dev54/native'
        report=json.loads((evidence/'native-instrument.json').read_text())
        with tempfile.TemporaryDirectory() as tmp:
            flags=['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core']
            boundary=Path(tmp)/'boundary';oracle=Path(tmp)/'oracle'
            subprocess.run([*flags,'tests/render_instrument_test.c',*SOURCES,'-o',str(boundary)],cwd=ROOT,check=True)
            subprocess.run([str(boundary)],check=True)
            subprocess.run([*flags,'tests/render_offset_test.c',*SOURCES,'src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c','-o',str(oracle)],cwd=ROOT,check=True)
            for name,data,meta in fixtures():
                case=report['cases'][name];trace=(evidence/(name+'.trace')).read_bytes()
                self.assertEqual((evidence/(name+'.mod')).read_bytes(),data)
                self.assertEqual(hashlib.sha256(data).hexdigest(),case['fixture_sha256'])
                self.assertEqual(hashlib.sha256(trace).hexdigest(),case['trace_sha256'])
                for repeat in range(2):self.assertEqual(decode_trace((evidence/(name+str(repeat)+'.log')).read_text(),meta['max_ticks']),(trace,'native-stop'))
                subprocess.run([str(oracle),str(evidence/(name+'.mod')),str(evidence/(name+'0.log')),'0'],check=True)
