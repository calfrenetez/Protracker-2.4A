"""Bounded HOST-only production format, syscall and CLI hardening campaign.

Run with unittest discovery. FH_EVIDENCE_DIR optionally retains logs/manifests;
otherwise all generated files live in a private TemporaryDirectory. No SDK/network.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import os
import platform
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
HERE = ROOT / 'tests/format_hardening'
CORE = ['document', 'pp20', 'safe_save', 'mod_project', 'mod_inspect', 'project', 'channels', 'pcm', 'wav', 'svx', 'raw']
ADAPTERS = ['file_save', 'file_load', 'project_import', 'mod_import', 'pp20_import']
SUPPORT = ['project_file','mod_file','sample_file','sample_raw_file','sample_svx_file']

class FormatHardening(unittest.TestCase):
    @classmethod
    def run_command(cls, argv, label, expected=0):
        start=time.monotonic()
        p=subprocess.run([str(x) for x in argv],cwd=ROOT,env=cls.env,capture_output=True,timeout=180)
        record={'id':label,'argv':[str(x) for x in argv],'cwd':'code/','returncode':p.returncode,'seconds':time.monotonic()-start,'expectedReturncode':expected}
        for field,body in [('stdout',p.stdout),('stderr',p.stderr)]:
            path=cls.evidence/(label+'.'+field+'.log');path.write_bytes(body);record[field]=path.name
        with (cls.evidence/'commands.jsonl').open('a') as f:f.write(json.dumps(record)+'\n')
        if p.returncode!=expected:
            raise AssertionError(f'{label}: exit {p.returncode}, expected {expected}\n'+p.stdout[-2000:].decode(errors='replace')+p.stderr[-6000:].decode(errors='replace'))
        return p

    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(prefix='pt24g-hardening-');cls.work=Path(cls.temp.name)
        cls.evidence=Path(os.environ.get('FH_EVIDENCE_DIR',str(cls.work/'evidence'))).resolve();cls.evidence.mkdir(parents=True,exist_ok=False)
        leaks=int(os.environ.get('FH_DETECT_LEAKS','0'))
        if leaks not in (0,1):raise ValueError('FH_DETECT_LEAKS must be 0 or 1')
        cls.env=os.environ.copy();cls.env.update(ASAN_OPTIONS=f'detect_leaks={leaks}:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1',PYTHONDONTWRITEBYTECODE='1')
        max_allocations=int(os.environ.get('FH_MAX_ALLOCATIONS','64'))
        max_cases=int(os.environ.get('FH_MAX_CASES','512'));cls.max_cases=max_cases
        if not 1<=max_allocations<=64 or not 1<=max_cases<=512:raise ValueError('campaign limits outside 1..64 allocations / 1..512 cases')
        flags=['cc',f'-DFH_MAX_ALLOCATIONS={max_allocations}','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-Isrc/core','-Isrc/platform','-Itests/format_hardening']
        cls.flags=flags
        spec=importlib.util.spec_from_file_location('fh_corpus',HERE/'make_corpus.py');gen=importlib.util.module_from_spec(spec);spec.loader.exec_module(gen)
        cls.corpus=cls.work/'corpus';cls.manifest=gen.generate(cls.corpus,max_cases=max_cases)
        (cls.evidence/'CORPUS_MANIFEST.json').write_text(json.dumps(cls.manifest,indent=2)+'\n')
        objects={}
        for directory,names in [('src/core',CORE),('src/platform',ADAPTERS+SUPPORT),('tests/format_hardening',['io_faults'])]:
            for name in names:
                obj=cls.work/(name+'.o');args=[*flags]
                if name in ADAPTERS:args+=['-include','tests/format_hardening/io_redirect.h']
                cls.run_command([*args,'-c',f'{directory}/{name}.c','-o',obj],'compile-'+name)
                objects[name]=obj
        core=[objects[n] for n in CORE]
        cls.corpus_exe=cls.work/'corpus-test'
        cls.run_command([*flags,HERE/'corpus_campaign.c',*core,'-o',cls.corpus_exe],'build-corpus')
        cls.io_exe=cls.work/'io-test'
        cls.run_command([*flags,HERE/'io_campaign.c',*objects.values(),'-o',cls.io_exe],'build-io')
        # CLI uses genuine, UNWRAPPED production adapters. Build separately.
        cls.cli=cls.work/'convert'
        cls.run_command([*flags,'tools/pt24g_convert.c',*[f'src/platform/{n}.c' for n in ADAPTERS if n!='file_load'],*[f'src/platform/{n}.c' for n in ['project_file','mod_file']],*core,'-o',cls.cli],'build-cli')
        cls.run_command(['cc','--version'],'compiler-version')
        (cls.evidence/'HOST_ENVIRONMENT.json').write_text(json.dumps({'os':platform.platform(),'architecture':platform.machine(),'python':platform.python_version(),'compiler':'literal cc','flags':flags,'ASAN_OPTIONS':cls.env['ASAN_OPTIONS'],'UBSAN_OPTIONS':cls.env['UBSAN_OPTIONS'],'leakSanitizer':'enabled' if leaks else 'disabled; explicit live-byte/pointer ledgers in C','hostSystem':platform.system(),'macOS':'CURRENT_HOST' if platform.system()=='Darwin' else 'NOT_RUN','Linux':'CURRENT_HOST' if platform.system()=='Linux' else 'NOT_RUN','native':'NOT_RUN','A1200':'NOT_RUN','Amiberry':'NOT_RUN','listening':'NOT_RUN'},indent=2)+'\n')

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def test_adversarial_corpus(self):
        p=self.run_command([self.corpus_exe,self.corpus],'corpus-campaign')
        self.assertIn(b'CORPUS_CAMPAIGN PASS',p.stdout)

    def test_real_io_fault_indices(self):
        d=self.work/'io';d.mkdir()
        p=self.run_command([self.io_exe,d,self.corpus],'io-campaign')
        self.assertIn(b'IO_CAMPAIGN PASS',p.stdout)
        self.assertEqual(list(d.iterdir()),[])
        warnings=p.stderr.decode().splitlines()
        self.assertEqual(len(warnings),32)  # two warnings for each of10 original +6 precision save paths
        self.assertTrue(all(line.startswith('Temporary file retained: ') for line in warnings))

    def test_cli_grammar_and_policy(self):
        d=self.work/'cli';d.mkdir()
        inputs={name:self.corpus/(name+'.ptg') for name in ['ptg-classic','ptg-16','ptg-24','ptg-rich','ptg-five','ptg-midi']}
        snapshots={p:p.read_bytes() for p in inputs.values()}
        cases=[]
        for mode in ['inspect','project','mod','mod8','mod8tpdf']:
            for count in range(5):
                valid=count==(1 if mode=='inspect' else 2)
                args=[mode,str(inputs['ptg-classic']),str(d/'out'),'extra','extra2'][:count+1]
                p=self.run_command([self.cli,*args],f'cli-arity-{mode}-{count}',0 if valid else 20)
                if valid and mode!='inspect':self.assertIn(b'verified=1 new_file=1',p.stdout);(d/'out').unlink()
                self.assertEqual(list(d.iterdir()),[])
                cases.append({'mode':mode,'argcAfterMode':count,'returncode':p.returncode})
        for mode in ['wav','iff','raw','bounce','render']:
            self.run_command([self.cli,mode,inputs['ptg-classic'],d/'out'],'cli-unknown-'+mode,20)
            self.assertFalse((d/'out').exists())
        for name,src in inputs.items():
            inspect=self.run_command([self.cli,'inspect',src],'cli-inspect-'+name)
            if name=='ptg-five':self.assertIn(b'required_strategy=BOUNCED',inspect.stdout)
            if name=='ptg-midi':self.assertIn(b'required_strategy=INCOMPLETE',inspect.stdout);self.assertIn(b'external-MIDI-audio-missing',inspect.stdout)
            if name=='ptg-classic':self.assertIn(b'required_strategy=LOSSLESS',inspect.stdout)
            for mode in ['mod','mod8','mod8tpdf']:
                valid=name=='ptg-classic' or (name in ('ptg-16','ptg-24') and mode!='mod')
                out=d/'out';p=self.run_command([self.cli,mode,src,out],f'cli-policy-{name}-{mode}',0 if valid else 20)
                if valid:
                    original=out.read_bytes();self.run_command([self.cli,mode,src,out],f'cli-existing-{name}-{mode}',20);self.assertEqual(out.read_bytes(),original);out.unlink()
                    self.run_command([self.cli,mode,src,out],f'cli-repeat-{name}-{mode}');self.assertEqual(out.read_bytes(),original);out.unlink()
                else:self.assertFalse(out.exists());self.assertIn(b'export refused',p.stderr)
                self.assertEqual(src.read_bytes(),snapshots[src]);self.assertEqual(list(d.iterdir()),[])
        for name in ['ptg-bad-crc.ptg','mod-bad-sig.mod','pp20-invalid-mod.pp20','wav-24-2.wav']:
            for mode in ['inspect','project','mod','mod8','mod8tpdf']:
                args=[self.cli,mode,self.corpus/name]
                if mode!='inspect':args.append(d/'out')
                p=self.run_command(args,'cli-reject-'+name+'-'+mode,20)
                self.assertIn(b'Input rejected',p.stderr);self.assertEqual(list(d.iterdir()),[])
        (self.evidence/'CLI_CASES.json').write_text(json.dumps(cases,indent=2)+'\n')

    def test_generator_reproducible_and_bounded(self):
        spec=importlib.util.spec_from_file_location('fh_again',HERE/'make_corpus.py');gen=importlib.util.module_from_spec(spec);spec.loader.exec_module(gen)
        second=self.work/'corpus-again';m=gen.generate(second,max_cases=self.max_cases)
        self.assertEqual(m,self.manifest)
        self.assertLessEqual(m['materializedBytes'],64<<20)
        for row in m['files']:
            data=(second/row['path']).read_bytes();self.assertEqual(hashlib.sha256(data).hexdigest(),row['sha256']);self.assertLessEqual(len(data),1<<20)
        self.assertEqual(len(m['cases']),len({r['id'] for r in m['cases']}))
