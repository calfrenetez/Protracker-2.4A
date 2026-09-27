from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class RenderInvertHandoff(unittest.TestCase):
    def test_reference_pcm_offline_and_incremental(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            sources=['render_invert','render','invert_bank','invert_sequence','invert_pcm','invert_loop','pitch','timeline','frame_clock','flow','voice','document','pp20','safe_save','mod_project','mod_inspect','project','channels','pcm']
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/render_invert_handoff_test.c',*[f'src/core/{s}.c' for s in sources],'-o',str(exe)],cwd=ROOT,check=True)
            cases=list((ROOT/'evidence/enhanced-editor/invert-handoff').glob('*.mod'))
            self.assertEqual(len(cases),4)
            commands=list((ROOT/'evidence/enhanced-editor/invert-handoff-commands').glob('*.mod'))
            self.assertEqual(len(commands),6)
            cases+=commands
            for mod in cases:subprocess.run([str(exe),str(mod),str(mod.with_suffix('.trace'))],check=True)
