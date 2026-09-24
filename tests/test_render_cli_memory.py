from pathlib import Path
import importlib.util
import os
import re
import subprocess
import tempfile
import unittest
from test_render_file import RENDER,IMPORT
from test_convert_cli import SOURCES as CONVERT
ROOT=Path(__file__).resolve().parents[1]
class RenderMemory(unittest.TestCase):
    def test_bounded_formats_and_every_allocation_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp);binary=d/'render';convert=d/'convert'
            flags=['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core']
            sources=['tests/render_cli_memory_test.c','src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c',*IMPORT,*RENDER]
            subprocess.run([*flags,*sources,'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([*flags,*CONVERT,'-o',str(convert)],cwd=ROOT,check=True)
            data=bytearray(1084+1024+131070);data[42:44]=(65535).to_bytes(2,'big');data[45]=64;data[950]=1;data[951]=127;data[1080:1084]=b'M.K.'
            for i in range(31):data[20+i*30+28:20+i*30+30]=(1).to_bytes(2,'big')
            data[1084:1088]=bytes([1,172,16,0])
            for i in range(131070):data[2108+i]=(i*37)&255
            source=d/'source.mod';source.write_bytes(data);project=d/'source.ptg';packed=d/'source.pp'
            subprocess.run([str(convert),'project',str(source),str(project)],check=True,capture_output=True)
            spec=importlib.util.spec_from_file_location('packer',ROOT/'tools/make_pp20_fixture.py');packer=importlib.util.module_from_spec(spec);spec.loader.exec_module(packer)
            packed.write_bytes(packer.literal(bytes(data)))
            options=['--pattern','0','--from-row','0','--to-row','1','--bits','24']
            expected=None
            for number,inp in enumerate([source,project,packed]):
                out=d/f'format{number}.wav'
                result=subprocess.run([str(binary),str(inp),str(out),*options],check=True,capture_output=True,text=True)
                self.assertIn('final=0',result.stdout)
                if expected is None:expected=out.read_bytes()
                self.assertEqual(out.read_bytes(),expected)
                print(inp.suffix+': '+result.stdout.splitlines()[-1])
            for stems in [False,True]:
                out=d/('stems' if stems else 'mix.wav');extra=['--stems'] if stems else []
                args=[str(binary),str(source),str(out),*options,*extra]
                result=subprocess.run(args,check=True,capture_output=True,text=True)
                calls=int(re.search(r'calls=(\d+)',result.stdout)[1]);print(('stems' if stems else 'mix')+': '+result.stdout.splitlines()[-1])
                preserved={p.name:p.read_bytes() for p in out.iterdir()} if stems else out.read_bytes()
                self.assertEqual(subprocess.run(args,capture_output=True).returncode,20)
                self.assertEqual({p.name:p.read_bytes() for p in out.iterdir()} if stems else out.read_bytes(),preserved)
                for fail in range(1,calls+1):
                    failed=d/f'fail-{stems}-{fail}';env=dict(os.environ,PT_TEST_FAIL=str(fail))
                    result=subprocess.run([str(binary),str(source),str(failed),*options,*extra],env=env,capture_output=True,text=True)
                    self.assertEqual(result.returncode,20,result.stdout+result.stderr);self.assertIn('final=0',result.stdout);self.assertFalse(failed.exists())
                self.assertFalse(list(d.glob('*.pttmp-*')))
                self.assertFalse(list(d.glob('*.ptstems-*')))
