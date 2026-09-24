from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tools/pt24g_convert.c','src/platform/file_load.c', 'src/platform/mod_import.c', 'src/platform/project_import.c', 'src/platform/pp20_import.c', 'src/platform/project_file.c', 'src/platform/mod_file.c',  'src/platform/file_save.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/safe_save.c',
           'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/project.c',
           'src/core/channels.c', 'src/core/pcm.c']
class ConvertCLI(unittest.TestCase):
    def test_new_file_publication_round_trip_and_existing_destination(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp); binary = str(d / 'PT24GConvert')
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core', *SOURCES, '-o', binary], cwd=ROOT, check=True)
            source = d / 'source.mod'; source.write_bytes((ROOT / 'evidence/baseline/mod.baseline').read_bytes())
            project = d / 'song.ptg'; restored = d / 'restored.mod'
            subprocess.run([binary, 'project', str(source), str(project)], check=True, capture_output=True)
            subprocess.run([binary, 'inspect', str(project)], check=True, capture_output=True)
            subprocess.run([binary, 'mod', str(project), str(restored)], check=True, capture_output=True)
            self.assertEqual(source.read_bytes(), restored.read_bytes())
            previous = project.read_bytes()
            result = subprocess.run([binary, 'project', str(source), str(project)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 20)
            self.assertEqual(previous, project.read_bytes())
            result = subprocess.run([binary, 'mod', str(project), str(source)], capture_output=True)
            self.assertEqual(result.returncode, 20)
            self.assertEqual(source.read_bytes(), restored.read_bytes())
            damaged = bytearray(previous); damaged[-1] ^= 1; project.write_bytes(damaged)
            result = subprocess.run([binary, 'mod', str(project), str(d / 'invalid.mod')], capture_output=True)
            self.assertEqual(result.returncode, 20)
            self.assertFalse((d / 'invalid.mod').exists())
            self.assertFalse(list(d.glob('*.pttmp-*')))

    def test_conversion_with_bounded_peak_memory(self):
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp);binary=d/'limited'
            sources=['tests/convert_memory_test.c',*SOURCES[1:]]
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Isrc/core',*sources,'-o',str(binary)],cwd=ROOT,check=True)
            data=bytearray(1084+1024+131070)
            data[42:44]=(65535).to_bytes(2,'big');data[45]=64;data[950]=1;data[951]=127;data[1080:1084]=b'M.K.'
            for i in range(31):data[20+i*30+28:20+i*30+30]=(1).to_bytes(2,'big')
            for i in range(131070):data[2108+i]=(i*37)&255
            source=d/'source.mod';source.write_bytes(data)
            for mode,suffix in [('mod','.mod'),('project','.ptg')]:
                output=d/(mode+suffix)
                result=subprocess.run([str(binary),mode,str(source),str(output)],capture_output=True,text=True,check=True)
                self.assertIn('final=0',result.stdout)
                print(mode+': '+result.stdout.splitlines()[-1])
                if mode=='mod':self.assertEqual(output.read_bytes(),data)
                self.assertEqual(source.read_bytes(),data)
            self.assertFalse(list(d.glob('*.pttmp-*')))
            # Unknown input is refused from bounded signature probes. Even a
            # large sparse file must not consume an encoded input allocation.
            unknown=d/'unknown.bin'
            with unknown.open('wb') as f:
                f.seek(64*1024*1024);f.write(b'\0')
            result=subprocess.run([str(binary),'project',str(unknown),str(d/'refused.ptg')],capture_output=True,text=True)
            self.assertEqual(result.returncode,20)
            self.assertIn('peak=0',result.stdout)
            self.assertFalse((d/'refused.ptg').exists())

            restored=d/'from-project.mod'
            result=subprocess.run([str(binary),'mod',str(d/'project.ptg'),str(restored)],capture_output=True,text=True,check=True)
            self.assertIn('final=0',result.stdout)
            self.assertEqual(restored.read_bytes(),data)
            print('project import: '+result.stdout.splitlines()[-1])

            import importlib.util
            spec=importlib.util.spec_from_file_location('packer',ROOT/'tools/make_pp20_fixture.py')
            packer=importlib.util.module_from_spec(spec);spec.loader.exec_module(packer)
            packed=d/'source.pp';packed.write_bytes(packer.literal(bytes(data)))
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-DPT_CONVERT_LIMIT=700000','-fsanitize=address,undefined','-Isrc/core',*sources,'-o',str(binary)],cwd=ROOT,check=True)
            unpacked=d/'unpacked.mod'
            result=subprocess.run([str(binary),'mod',str(packed),str(unpacked)],capture_output=True,text=True,check=True)
            self.assertEqual(unpacked.read_bytes(),data)
            self.assertIn('final=0',result.stdout)
            print('packed import: '+result.stdout.splitlines()[-1])
