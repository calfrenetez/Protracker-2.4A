"""Exact current recording ownership fixtures; use only after fresh shared030 coordination."""
import argparse,fcntl,hashlib,json,os,shutil,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
sys.path.insert(0,str(INFRA/'scripts'))
from shared_guest import Guest
from emulator import processes

def sha(data):return hashlib.sha256(data).hexdigest()
def verified(build):
 manifest=json.loads((build/'manifest.json').read_text())
 tree=subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=ROOT,text=True).strip()
 cases=[];inputs={}
 for label,name,marker in (
  ('capture','PTExecCaptureTest','CAPTURE STAGING PASS:'),
  ('editor-capture','PTExecEditorCaptureTest','EDITOR CAPTURE PASS:')):
  record=manifest['targets'][name];binary=build/name
  if binary.stat().st_size!=record['binary_bytes'] or sha(binary.read_bytes())!=record['binary_sha256']:
   raise RuntimeError('Exact binary changed: '+name)
  for path,expected in record['dependencies'].items():
   if path in inputs and inputs[path]!=expected:raise RuntimeError('Conflicting source hashes: '+path)
   inputs[path]=expected
  cases.append({'label':label,'binary':name,'path':str(binary),'bytes':record['binary_bytes'],'sha256':record['binary_sha256'],'marker':marker})
 for path,expected in inputs.items():
  if sha(subprocess.check_output(['git','show',tree+':'+path],cwd=ROOT))!=expected:
   raise RuntimeError('Committed source changed: '+path)
 cc=Path(manifest['compiler'])
 if sha(cc.read_bytes())!=manifest['compiler_sha256']:raise RuntimeError('Compiler changed')
 for name,expected in manifest['runtime_inputs'].items():
  path=Path(subprocess.check_output([str(cc),'-m68000','-msoft-float','-mcrt=nix20','-print-file-name='+name],text=True).strip())
  if sha(path.read_bytes())!=expected:raise RuntimeError('Runtime changed: '+name)
 return tree,cases,manifest

def idle(guest):
 status=guest.command('GET_STATUS');audio=guest.command('GET_AUDIO_STATE')
 if 'Paused=false' not in status.split('\t') or not all('ch%d_dma=0'%i in audio.split('\t') for i in range(4)):
  raise RuntimeError('Running DMA-off guard failed')
 return {'status':status,'audio':audio}

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('build',type=Path);parser.add_argument('--verify-only',action='store_true')
 args=parser.parse_args();deadline=time.monotonic()+350
 tree,cases,manifest=verified(args.build.resolve())
 if args.verify_only:
  print(json.dumps({'passed':True,'source_tree':tree,'cases':cases},indent=2));return
 out=ROOT/'build/dev'/('capture-current-qualification-'+str(time.time_ns()));out.mkdir()
 result={'passed':False,'scope':'Two current native recording format/transfer and editor ownership fixtures; injected input, no card/input MMIO, physical, timing or listening acceptance','source_tree':tree,'verified_manifest':manifest,'cases':[]}
 print('EVIDENCE',out,flush=True)
 try:
  runners={}
  for case in cases:
   runner=out/('runner-'+case['label']);runner.mkdir()
   with subprocess.Popen(['git','archive',tree,'tools'],cwd=ROOT,stdout=subprocess.PIPE) as export:
    subprocess.run(['tar','-x','-C',str(runner)],stdin=export.stdout,check=True);export.stdout.close()
    if export.wait():raise RuntimeError('Runner export failed')
   artifacts=runner/'build/dev';artifacts.mkdir(parents=True)
   shutil.copyfile(case['path'],artifacts/case['binary'])
   if sha((artifacts/case['binary']).read_bytes())!=case['sha256']:raise RuntimeError('Private candidate copy changed')
   runners[case['label']]=runner
  with (INFRA/'runtime/test.lock').open('a') as lock:
   fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB);rows=processes()
   if len(rows)!=1 or rows[0][0]!=19081:raise RuntimeError('Original sole emulator changed')
   result['original_processes']=rows;result['initial_idle']=idle(Guest(INFRA,out))
  for case in cases:
   if deadline-time.monotonic()<155:raise RuntimeError('Insufficient bounded window; no further launch')
   with (INFRA/'runtime/test.lock').open('a') as lock:
    fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    if processes()!=rows:raise RuntimeError('Original process changed before launch')
    idle(Guest(INFRA,out))
   if deadline-time.monotonic()<155:raise RuntimeError('Insufficient bounded window after live guards; no launch')
   runner=runners[case['label']]
   with (out/(case['label']+'-runner.log')).open('w') as log:
    rc=subprocess.run([str(INFRA/'.venv/bin/python'),'tools/shared_infra_render_files.py','--'+case['label']+'-memory'],cwd=runner,stdout=log,stderr=subprocess.STDOUT,timeout=140).returncode
   raw=(out/(case['label']+'-runner.log')).read_text()
   if rc:raise RuntimeError(case['label']+' runner failed; preserve evidence and stop')
   run=Path(raw.strip().splitlines()[-1]);record=json.loads((run/'result.json').read_text())
   if not record.get('passed') or not record.get('run_files_cleaned') or record[case['label']+'_returncode']!='0' or record[case['binary']+'_sha256']!=case['sha256']:
    raise RuntimeError('Native pass/cleanup/hash unconfirmed')
   native_log=(run/(case['label']+'.log')).read_text()
   if case['marker'] not in native_log or 'EXEC MEMORY PASS:' not in native_log or 'zero owned bytes' not in native_log:
    raise RuntimeError('Fixture/allocator-zero marker missing')
   if case['label']=='editor-capture' and not all(marker in native_log for marker in ('CAPTURE SESSION PASS:','AMIGUS CAPTURE PASS:')):
    raise RuntimeError('Nested format/reservation/session assertion markers missing')
   with (INFRA/'runtime/test.lock').open('a') as lock:
    fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    if processes()!=rows:raise RuntimeError('Original process changed after run')
    guest=Guest(INFRA,run);independent={'passed':False,'processes':rows,**idle(guest),'observations':[]}
    for i in range(11):
     paths={str(path):os.path.lexists(path) for path in (guest.share/run.name,guest.launch)}
     independent['observations'].append(paths)
     if any(paths.values()):raise RuntimeError('Independent exact paths reappeared; no retry')
     if i<10:time.sleep(1)
    if processes()!=rows:raise RuntimeError('Original process changed during check')
    independent['final_idle']=idle(guest);independent['passed']=True
    (run/'independent-cleanup.json').write_text(json.dumps(independent,indent=2)+'\n')
   result['cases'].append({'candidate':case,'run':str(run),'result':record,'independent_cleanup':independent,'native_log':native_log})
   (out/'qualification-result.json').write_text(json.dumps(result,indent=2)+'\n')
   print(case['label'],'PASS',run,flush=True)
  if time.monotonic()>deadline:raise RuntimeError('Qualification window exceeded after guarded cleanup')
  result['passed']=True
 except Exception as error:
  result['error']=str(error);raise
 finally:
  (out/'qualification-result.json').write_text(json.dumps(result,indent=2)+'\n');print(out,flush=True)
if __name__=='__main__':main()
