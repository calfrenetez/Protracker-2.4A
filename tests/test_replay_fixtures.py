from pathlib import Path
import importlib.util
import json
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class ReplayFixtures(unittest.TestCase):
    def test_generated_inputs_are_valid_and_reproducible(self):
        spec=importlib.util.spec_from_file_location('flow',ROOT/'tools/make_replay_flow_fixtures.py');flow=importlib.util.module_from_spec(spec);spec.loader.exec_module(flow)
        cases=list(flow.fixtures());self.assertEqual(cases,list(flow.fixtures()));self.assertEqual(len(cases),16)
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);checker=tmp/'check';out=tmp/'fixtures'
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tools/modcheck.c','src/core/mod_inspect.c','-o',str(checker)],cwd=ROOT,check=True)
            subprocess.run(['python3',str(ROOT/'tools/make_replay_flow_fixtures.py'),str(out)],check=True)
            manifest=json.loads((out/'manifest.json').read_text());self.assertEqual(len(manifest['cases']),16)
            for name,data,metadata in cases:
                path=out/(name+'.mod');self.assertEqual(path.read_bytes(),data);self.assertEqual(metadata['native_trace'],'NOT RUN')
                result=subprocess.run([str(checker),str(path)],capture_output=True,text=True,check=True)
                self.assertIn('orders=2 patterns=2',result.stdout);self.assertIn('warnings=0x0',result.stdout)
                for pattern,row,channel,effect,parameter in metadata['events']:
                    offset=1084+(pattern*64*4+row*4+channel)*4
                    self.assertEqual(data[offset+2]&15,effect);self.assertEqual(data[offset+3],parameter)
            # Reusing a directory cannot overwrite a previous fixture/evidence set.
            result=subprocess.run(['python3',str(ROOT/'tools/make_replay_flow_fixtures.py'),str(out)],capture_output=True)
            self.assertNotEqual(result.returncode,0)
