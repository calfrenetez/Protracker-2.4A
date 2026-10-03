"""Exact bounded per-bit Paula fixtures, only after fresh shared030 coordination."""
import fcntl,hashlib,json,os,shutil,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
SPLIT=Path('/private/tmp/protracker-paula-song-split-1di4u5r1')
OLD=Path('/private/tmp/protracker-paula-incremental-4glly2eg')
CC=Path('/Users/james1/Documents/Codex/2026-08-20/work-from-the-design-spec-v1/repo/.cache/amiga/bin/m68k-amigaos-gcc')
sys.path.insert(0,str(INFRA/'scripts'))
from shared_guest import Guest
from emulator import processes

def sha(data):return hashlib.sha256(data).hexdigest()
def verified():
 split=json.loads((SPLIT/'manifest.json').read_text())
 old=json.loads((OLD/'build.json').read_text())
 tree=subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=ROOT,text=True).strip()
 cases=[]
 for bits in (8,16,24):
  name='PTExecPaulaSong'+str(bits)+'Test';r=split['targets'][name]
  cases.append({'label':'song-'+str(bits),'argument':'song','bits':bits,'binary':name,'alias':'PTExecPaulaSongTest','path':str(SPLIT/name),'bytes':r['binary_bytes'],'sha256':r['binary_sha256'],'inputs':r['dependencies']})
 r=old['binaries']['PTExecEditorPaulaTest']
 cases.append({'label':'editor','argument':'editor','bits':None,'binary':'PTExecEditorPaulaTest','alias':'PTExecEditorPaulaTest',**r})
 inputs={}
 for case in cases:
  p=Path(case['path'])
  if p.stat().st_size!=case['bytes'] or sha(p.read_bytes())!=case['sha256']:raise RuntimeError('Exact binary changed: '+case['binary'])
  for name,expected in case['inputs'].items():
   if name in inputs and inputs[name]!=expected:raise RuntimeError('Conflicting input hashes: '+name)
   inputs[name]=expected
 for name,expected in inputs.items():
  if sha(subprocess.check_output(['git','show',tree+':'+name],cwd=ROOT))!=expected:raise RuntimeError('Committed source changed: '+name)
 if split['compiler_sha256']!=old['compiler_sha256'] or sha(CC.read_bytes())!=split['compiler_sha256']:raise RuntimeError('Compiler mismatch')
 if split['runtime_inputs']!=old['runtime_inputs']:raise RuntimeError('Runtime manifests disagree')
 for name,expected in split['runtime_inputs'].items():
  p=Path(subprocess.check_output([str(CC),'-m68000','-msoft-float','-mcrt=nix20','-print-file-name='+name],text=True).strip())
  if sha(p.read_bytes())!=expected:raise RuntimeError('Runtime changed: '+name)
 return tree,cases,split,old

def idle(guest):
 status=guest.command('GET_STATUS');audio=guest.command('GET_AUDIO_STATE')
 if 'Paused=false' not in status.split('\t') or not all('ch%d_dma=0'%i in audio.split('\t') for i in range(4)):raise RuntimeError('Running DMA-off guard failed')
 return {'status':status,'audio':audio}

def main():
 deadline=time.monotonic()+650
 tree,cases,split,old=verified()
 if '--verify-only' in sys.argv:
  print(json.dumps({'passed':True,'source_tree':tree,'cases':[{k:v for k,v in c.items() if k!='inputs'} for c in cases]},indent=2));return
 out=ROOT/'build/dev'/('paula-split-qualification-'+str(time.time_ns()));out.mkdir()
 result={'passed':False,'scope':'Three native full per-bit song matrices plus editor ownership fixture; injected voices, no device/physical/timing acceptance','source_tree':tree,'cases':[], 'verified_split_manifest':split,'verified_editor_manifest':old['binaries']['PTExecEditorPaulaTest']}
 print('EVIDENCE',out,flush=True)
 try:
  # All immutable exports and exact alias copies are prepared before guest access.
  runners={}
  for case in cases:
   runner=out/('runner-'+case['label']);runner.mkdir()
   with subprocess.Popen(['git','archive',tree,'tools'],cwd=ROOT,stdout=subprocess.PIPE) as export:
    subprocess.run(['tar','-x','-C',str(runner)],stdin=export.stdout,check=True);export.stdout.close()
    if export.wait():raise RuntimeError('Runner export failed')
   artifacts=runner/'build/dev';artifacts.mkdir(parents=True)
   shutil.copyfile(case['path'],artifacts/case['alias'])
   if sha((artifacts/case['alias']).read_bytes())!=case['sha256']:raise RuntimeError('Private alias mismatch')
   runners[case['label']]=runner
  with (INFRA/'runtime/test.lock').open('a') as lock:
   fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
   rows=processes()
   if len(rows)!=1 or rows[0][0]!=19081:raise RuntimeError('Original sole emulator changed')
   result['original_processes']=rows
   guest=Guest(INFRA,out);result['initial_idle']=idle(guest)
  for case in cases:
   label=case['label']
   if deadline-time.monotonic()<155:raise RuntimeError('Insufficient bounded window; no further launch')
   with (INFRA/'runtime/test.lock').open('a') as lock:
    fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    if processes()!=rows:raise RuntimeError('Original process changed before launch')
    idle(Guest(INFRA,out))
   runner=runners[label]
   with (out/(label+'-runner.log')).open('w') as log:
    rc=subprocess.run([str(INFRA/'.venv/bin/python'),'tools/shared_infra_render_files.py','--paula-memory',case['argument']],cwd=runner,stdout=log,stderr=subprocess.STDOUT,timeout=140).returncode
   raw=(out/(label+'-runner.log')).read_text()
   if rc:raise RuntimeError(label+' runner failed; preserve run and stop')
   run=Path(raw.strip().splitlines()[-1]);record=json.loads((run/'result.json').read_text())
   if not record.get('passed') or not record.get('run_files_cleaned') or record[case['alias']+'_sha256']!=case['sha256']:raise RuntimeError('Native pass/cleanup/hash unconfirmed')
   native_log=(run/('editor-paula.log' if case['argument']=='editor' else 'paula-song.log')).read_text()
   if 'EXEC MEMORY PASS:' not in native_log or 'zero owned bytes' not in native_log:raise RuntimeError('Final allocator-zero marker missing')
   if case['bits'] is not None:
    markers=[line for line in native_log.splitlines() if line.startswith('PAULA SONG CASE PASS:')]
    if markers!=['PAULA SONG CASE PASS: bits='+str(case['bits'])+' full per-bit assertions and runtime bounds']:raise RuntimeError('Selected depth marker mismatch')
   with (INFRA/'runtime/test.lock').open('a') as lock:
    fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    if processes()!=rows:raise RuntimeError('Original process changed after run')
    guest=Guest(INFRA,run);independent={'passed':False,'processes':rows,**idle(guest),'observations':[]}
    for i in range(11):
     paths={str(p):os.path.lexists(p) for p in (guest.share/run.name,guest.launch)}
     independent['observations'].append(paths)
     if any(paths.values()):raise RuntimeError('Independent exact paths reappeared; no retry')
     if i<10:time.sleep(1)
    if processes()!=rows:raise RuntimeError('Original process changed during check')
    independent['final_idle']=idle(guest);independent['passed']=True
    (run/'independent-cleanup.json').write_text(json.dumps(independent,indent=2)+'\n')
   result['cases'].append({'candidate':{k:v for k,v in case.items() if k!='inputs'},'run':str(run),'result':record,'independent_cleanup':independent,'native_log':native_log})
   (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
   print(label,'PASS',run,flush=True)
  result['passed']=True
 except Exception as error:
  result['error']=str(error);raise
 finally:
  (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(out,flush=True)
if __name__=='__main__':main()
