from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_unused_fixtures import fixtures
from test_porta_emulator import decode_trace
COMMON=['src/core/pitch.c','src/core/flow.c','src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
class UnusedNative(unittest.TestCase):
    def test_native_stored_output_periods_and_pcm(self):
        evidence=ROOT/'evidence/enhanced-editor/dev55/native'
        report=json.loads((evidence/'native-unused.json').read_text())
        baseline,_=decode_trace((evidence/'baseline.log').read_text(),160)
        self.assertEqual(b''.join(baseline[i:i+36] for i in range(0,len(baseline),52)),(ROOT/'evidence/enhanced-editor/dev28/native/speed.trace').read_bytes())
        with tempfile.TemporaryDirectory() as tmp:
            state=Path(tmp)/'pitch';pcm=Path(tmp)/'pcm'
            flags=['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core']
            subprocess.run([*flags,'tests/pitch_test.c',*COMMON,'-o',str(state)],cwd=ROOT,check=True)
            subprocess.run([*flags,'tests/render_porta_test.c','src/core/render.c','src/core/timeline.c','src/core/frame_clock.c','src/core/voice.c',*COMMON,'-o',str(pcm)],cwd=ROOT,check=True)
            subprocess.run([str(state)],check=True)
            self.assertEqual(subprocess.check_output([str(pcm)],text=True),(evidence/'safety.log').read_text())
            for name,data,metadata in fixtures():
                case=report['cases'][name];trace=(evidence/(name+'.trace')).read_bytes()
                self.assertEqual((evidence/(name+'.mod')).read_bytes(),data)
                self.assertEqual(hashlib.sha256(data).hexdigest(),case['fixture_sha256'])
                self.assertEqual(hashlib.sha256(trace).hexdigest(),case['trace_sha256'])
                for repeat in range(2):
                    self.assertEqual(decode_trace((evidence/f'{name}{repeat}.log').read_text(),metadata['max_ticks']),(trace,'native-stop'))
                for binary,kind in [(state,'pitch'),(pcm,'pcm')]:
                    result=subprocess.check_output([str(binary),str(evidence/(name+'.mod')),str(evidence/(name+'0.log'))],text=True)
                    self.assertEqual(result,(evidence/(name+'-'+kind+'.log')).read_text())
            print("UNUSED: six repeated native traces match host/m68k stored/output words and PCM for8xx/E8x")
