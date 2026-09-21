from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['src/core/document.c','src/core/pp20.c','src/core/safe_save.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
class ModRound8(unittest.TestCase):
    def test_core_and_explicit_new_file_policy(self):
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp);test=d/'test';cli=d/'convert'
            flags=['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core']
            subprocess.run([*flags,'tests/mod_round8_test.c',*SOURCES,'-o',str(test)],cwd=ROOT,check=True)
            subprocess.run([str(test),str(ROOT/'evidence/baseline/mod.baseline'),str(d)],check=True)
            subprocess.run([*flags,'tools/pt24g_convert.c','src/platform/file_save.c',*SOURCES,'-o',str(cli)],cwd=ROOT,check=True)
            original=(d/'high.ptg').read_bytes();output=d/'output.mod'
            args=[str(cli),'mod',str(d/'high.ptg'),str(output)]
            result=subprocess.run(args,capture_output=True,text=True)
            self.assertEqual(result.returncode,20);self.assertFalse(output.exists())
            args[1]='mod8';result=subprocess.run(args,capture_output=True,text=True,check=True)
            self.assertIn('dither=none result=CONVERTED remaining_issues=0x0',result.stdout)
            self.assertEqual(output.read_bytes(),(d/'expected.mod').read_bytes())
            self.assertEqual(subprocess.run(args,capture_output=True).returncode,20)
            self.assertEqual(output.read_bytes(),(d/'expected.mod').read_bytes());self.assertEqual((d/'high.ptg').read_bytes(),original)
            args[1]='mod8tpdf';args[3]=str(d/'tpdf.mod')
            result=subprocess.run(args,capture_output=True,text=True,check=True)
            self.assertIn('dither=tpdf-fixed result=CONVERTED',result.stdout)
            self.assertEqual((d/'tpdf.mod').read_bytes(),(d/'dithered.mod').read_bytes())
            self.assertNotEqual((d/'tpdf.mod').read_bytes(),output.read_bytes())
            self.assertEqual(subprocess.run(args,capture_output=True).returncode,20)
            self.assertEqual((d/'high.ptg').read_bytes(),original)
            self.assertFalse(list(d.glob('*.pttmp-*')))
