from pathlib import Path
import hashlib
import json
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/flow_test.c','src/core/flow.c','src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
class Flow(unittest.TestCase):
    def test_native_trace_parity_and_extended_boundaries(self):
        evidence=ROOT/'evidence/enhanced-editor/dev28/native'
        report=json.loads((evidence/'native-flow.json').read_text())
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'flow'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
            for name,case in report['cases'].items():
                mod=evidence/(name+'.mod');trace=evidence/(name+'.trace')
                self.assertEqual(hashlib.sha256(mod.read_bytes()).hexdigest(),case['fixture_sha256'])
                self.assertEqual(hashlib.sha256(trace.read_bytes()).hexdigest(),case['trace_sha256'])
                result=subprocess.run([str(binary),str(mod),str(trace),str(case['max_ticks'])],check=True,capture_output=True,text=True)
                self.assertIn(f'ticks={case["ticks"]} reason={case["reason"]}',result.stdout)
            print('FLOW: all 16 native trace fixtures match portable control flow')
