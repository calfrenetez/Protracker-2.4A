"""Finite host-only editor checks; first results retained, no target access."""
from pathlib import Path
import ast,hashlib,json,os,subprocess,sys,time
BASE=Path(__file__).resolve().parent
SOURCE=BASE/'candidate'
OUTPUT=BASE/'host'
sys.dont_write_bytecode=True
assert not OUTPUT.exists()
assert not any(n in os.environ for n in ('CPATH','C_INCLUDE_PATH','LIBRARY_PATH','ASAN_OPTIONS','UBSAN_OPTIONS'))
OUTPUT.mkdir()
seal=json.loads((BASE/'source-controls.json').read_text())
def inventory():return {p.relative_to(SOURCE).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(SOURCE.rglob('*')) if p.is_file()}
def constant(file,key):return next(ast.literal_eval(n.value) for n in ast.parse((SOURCE/file).read_text()).body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==key for t in n.targets))
def write(): (OUTPUT/'execution.json').write_text(json.dumps(result,indent=2)+'\n')
assert inventory()==seal['source_files']
sys.path.insert(0,str(SOURCE/'tests'))
from test_mixed_owner import SOURCES as mixed
from test_mixed_owner_established import SOURCES as established
CHECKED=constant('tests/test_editor_mixed_checked.py','CHECKED')
WORKFLOW=constant('tests/test_editor_mixed_establish.py','WORKFLOW')
editor=list(dict.fromkeys(constant('tests/test_editor.py','SOURCES')[1:]+constant('tests/test_editor_studio.py','EXTRA')+constant('tests/test_wavetable_dispatch.py','DISPATCH')+['src/editor/sampler_wavetable.c']+['src/core/'+n+'.c' for n in ['amigus_reservation','amigus_wavetable_cache','amigus_sample_ram','sample_cache','playback_pcm']]+['src/editor/editor_wavetable.c','src/platform/sample_import.c','src/platform/raw_import.c','src/platform/mod_import.c','src/platform/pp20_import.c']))
result={'status':'RUNNING','scope':seal['scope'],'calls':[],'source_before':inventory()};write()
try:
 groups=[('bridges',['tests/editor_mixed_bridges_test.c','src/editor/editor_mixed_bridges.c',*CHECKED,*mixed[1:],*editor,*WORKFLOW]),('checked-standalone',['tests/editor_mixed_checked_test.c',*CHECKED,*mixed[1:],*editor,*WORKFLOW]),('original-established',established)]
 for label,inputs in groups:
  binary=OUTPUT/label
  units=list(dict.fromkeys(inputs))
  commands=[['/usr/bin/cc','-std=c99','-UNDEBUG','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*units,'-o',str(binary)],[str(binary)]]
  for stage,argv in zip(('compile','run'),commands):
   record={'label':label,'stage':stage,'argv':argv,'cwd':str(SOURCE),'timeout_seconds':120 if stage=='compile' else 45}
   result['calls'].append(record);write();start=time.monotonic()
   with (OUTPUT/(label+'-'+stage+'.stdout')).open('wb') as out,(OUTPUT/(label+'-'+stage+'.stderr')).open('wb') as err:
    try:
     proc=subprocess.run(argv,cwd=SOURCE,stdout=out,stderr=err,timeout=record['timeout_seconds'],check=False)
     record['returncode']=proc.returncode
    finally:record['elapsed_seconds']=time.monotonic()-start;write()
   if proc.returncode:raise RuntimeError('First failure retained: '+label+' '+stage)
   if stage=='compile':record['product']={'bytes':binary.stat().st_size,'sha256':hashlib.sha256(binary.read_bytes()).hexdigest()};write()
 result['status']='PASS_HOST_ONLY'
except BaseException as failure:
 result['status']='FIRST_FAILURE_RETAINED';result['failure']=repr(failure);raise
finally:
 result['source_after']=inventory();result['source_stable']=result['source_after']==result['source_before'];write()
print(json.dumps({'status':result['status'],'calls':len(result['calls']),'source_stable':result['source_stable']}))
