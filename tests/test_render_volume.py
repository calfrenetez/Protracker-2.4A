from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_volume_fixtures import fixtures
from test_flow_emulator import decode_trace
SOURCES=['tests/render_volume_test.c','src/core/render.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c','src/core/voice.c','src/core/project.c','src/core/channels.c','src/core/pcm.c','src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c']
class VolumeRender(unittest.TestCase):
    def test_pinned_native_volume_and_silent_phase(self):
        evidence=ROOT/'evidence/enhanced-editor/dev34/native'
        report=json.loads((evidence/'native-volume.json').read_text())
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'volume'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
            for name,data,metadata in fixtures():
                case=report['cases'][name];trace=(evidence/(name+'.trace')).read_bytes()
                self.assertEqual((evidence/(name+'.mod')).read_bytes(),data)
                self.assertEqual(hashlib.sha256(data).hexdigest(),case['fixture_sha256'])
                self.assertEqual(hashlib.sha256(trace).hexdigest(),case['trace_sha256'])
                for repeat in range(2):
                    self.assertEqual(decode_trace((evidence/f'{name}{repeat}.log').read_text(),metadata['max_ticks']),(trace,'native-stop'))
                result=subprocess.check_output([str(binary),str(evidence/(name+'.mod')),str(evidence/(name+'0.log'))],text=True)
                self.assertEqual(result,(evidence/(name+'-pcm.log')).read_text())
            print('VOLUME: seven repeated pinned native traces match every host/m68k rendered PCM frame')
