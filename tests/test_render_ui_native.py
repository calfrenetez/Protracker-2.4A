"""Keep the native UI's retained finetune/WAV result tied to the current renderer."""
from pathlib import Path
import hashlib,json,subprocess,tempfile,unittest
from test_render_file import RENDER,IMPORT
ROOT=Path(__file__).resolve().parents[1]
class NativeRenderUI(unittest.TestCase):
    def test_native_finetune_wav_and_project_identity(self):
        e=ROOT/'evidence/enhanced-editor/dev58/native'
        report=json.loads((e/'native-render-ui.json').read_text())
        self.assertTrue(report['full_render_workflow_validated'])
        self.assertTrue(report['finetune_export_exact'])
        fixture=ROOT/'evidence/enhanced-editor/dev28/native/speed.mod'
        self.assertEqual(hashlib.sha256(fixture.read_bytes()).hexdigest(),report['fixture_sha256'])
        tuned=bytearray(fixture.read_bytes());self.assertEqual(tuned[44],0);tuned[44]=1
        self.assertEqual(bytes(tuned),(e/'finetune.mod').read_bytes())
        self.assertEqual((e/'baseline.ptg').read_bytes(),(e/'saved.ptg').read_bytes())
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp);cli=d/'render'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tools/pt24g_render.c','src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c',*IMPORT,*RENDER,'-o',str(cli)],cwd=ROOT,check=True)
            for name,source in [('render',fixture),('finetune',e/'finetune.mod')]:
                wav=d/(name+'.wav')
                subprocess.run([str(cli),str(source),str(wav),'--rate','44100','--bits','16','--gain','65536'],check=True,capture_output=True)
                self.assertEqual(wav.read_bytes(),(e/(name+'.wav')).read_bytes())
            self.assertEqual((e/'pattern.wav').read_bytes(),(e/'render.wav').read_bytes())
            self.assertNotEqual((e/'finetune.wav').read_bytes(),(e/'render.wav').read_bytes())
