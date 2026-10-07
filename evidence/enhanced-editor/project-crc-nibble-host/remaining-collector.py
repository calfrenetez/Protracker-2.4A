"""Distinct HOST completion of mixed65 only; preserve failed first collector."""
import hashlib,json,os,signal,subprocess,time
from pathlib import Path
BASE=Path(__file__).resolve().parent
REPO=Path('/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A')
FIRST=BASE/'root-project-crc-nibble-host-first-v1'
PROPOSAL=BASE/'native-project-crc-nibble-source-proposal-v1'
OUT=BASE/'root-project-crc-nibble-mixed65-completion-first-v1'
tree=FIRST/'tree'
def pin(p):
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'mode':p.stat().st_mode}
def inventory(root):
 paths=[p for d in ('src','tests') for p in (root/d).rglob('*') if p.is_file()]
 assert len(paths)<=2048 and all(not p.is_symlink() and p.stat().st_size<=16777216 for p in paths)
 return {str(p.relative_to(root)):pin(p) for p in paths}
receipt=FIRST/'receipt.json'
assert pin(receipt)=={'bytes':439061,'sha256':'efa7a5fe8383f36de0464dfea5c48d4ab802b1ced7530666982aa501fb1ea620','mode':33188}
prior=json.loads(receipt.read_text())
assert prior['status']=='FIRST_FAILURE_RETAINED_NO_RETRY' and len(prior['commands'])==11
assert prior['target_operation'] is False and prior['automatic_retry'] is False
retained={'receipt.json':pin(receipt)}
for i,e in enumerate(prior['commands']):
 assert e['returncode']==(0 if i<10 else 1) and e['reaped'] is True and e['group_absent'] is True and e['errors']==[]
 assert e['passed'] is (i<10)
 for n in ('stdout','stderr'):
  f=FIRST/(e['label']+'.'+n);assert pin(f)==e['streams'][n];retained[f.name]=pin(f)
assert (FIRST/'mixed65-compile.stderr').read_bytes()==b'tests/native_mixed_ram_port_test.c:6:10: fatal error: \'native_mixed_ram_port.h\' file not found\n    6 | #include "native_mixed_ram_port.h"\n      |          ^~~~~~~~~~~~~~~~~~~~~~~~~\n1 error generated.\n'
assert inventory(tree)==prior['selected_tree_before'] and inventory(REPO)==prior['repository_before']
assert all(pin(PROPOSAL/n)==v for n,v in prior['proposal_before'].items())
assert all(pin(REPO/n)==v for n,v in prior['protected_before'].items())
assert pin(Path('/usr/bin/cc'))==prior['compiler_before']
old=BASE/'root-mixed-ram-diagnostic-host-first-v1/receipt.json'
assert pin(old)=={'bytes':311986,'sha256':'78753bcd71b26d66759fe09f951dbefa10b8b4b7cf41ebbaf965d92fff5b35be','mode':33188}
oracle=json.loads(old.read_text())['actual_oracle'].encode('ascii')
argv=list(prior['commands'][-1]['args']);assert argv[0]=='/usr/bin/cc' and argv[-2]=='-o'
assert '-Isrc/native/readers_ram' not in argv
argv.insert(argv.index('-I.'),'-Isrc/native/readers_ram')
argv[-1]=str(OUT/'mixed65')
OUT.mkdir()
result={'scope':'HOST_ONLY_REMAINING_MIXED65_COMPILER_INCLUDE_FIX','status':'NOT_COMPLETE','first_attempt':pin(receipt),'first_attempt_preserved':True,'sole_recipe_change':'add -Isrc/native/readers_ram from existing test recipe; distinct output executable','first_retained_before':retained,'selected_tree_before':prior['selected_tree_before'],'repository_before':prior['repository_before'],'proposal_before':prior['proposal_before'],'protected_before':prior['protected_before'],'compiler_before':prior['compiler_before'],'commands':[],'target_operation':False,'automatic_retry':False,'crc_groups_repeated':False}
def save():(OUT/'receipt.json').write_text(json.dumps(result,indent=2)+'\n')
def call(label,args,limit,environment):
 e={'label':label,'args':args,'timeout_seconds':limit,'passed':False,'errors':[]};result['commands'].append(e);save();p=None;start=time.monotonic()
 def probe():
  try:os.killpg(p.pid,0);return True
  except ProcessLookupError:return False
  except BaseException as error:e['errors'].append({'phase':'group probe','error':str(error)});return None
 try:
  with (OUT/(label+'.stdout')).open('wb') as out,(OUT/(label+'.stderr')).open('wb') as err:
   p=subprocess.Popen(args,cwd=tree,env=environment,stdout=out,stderr=err,start_new_session=True);e['pid']=p.pid;save()
   try:p.wait(timeout=limit)
   except BaseException as error:e['errors'].append({'phase':'bounded wait','error':str(error)})
 except BaseException as error:e['errors'].append({'phase':'invoke','error':str(error)})
 finally:
  if p is not None:
   try:
    if probe() is not False:
     e['errors'].append({'phase':'finalize','error':'Owned HOST group remains or unknown'})
     for sig in (signal.SIGTERM,signal.SIGKILL):
      if probe() is False:break
      if probe() is True:
       try:os.killpg(p.pid,sig)
       except ProcessLookupError:pass
       except BaseException as error:e['errors'].append({'phase':'HOST group signal','error':str(error)})
      until=time.monotonic()+.5
      while time.monotonic()<until:
       p.poll()
       if probe() is False:break
       time.sleep(.02)
    try:p.wait(timeout=1)
    except BaseException as error:e['errors'].append({'phase':'bounded reap','error':str(error)})
   except BaseException as error:e['errors'].append({'phase':'finalization','error':str(error)})
   e.update(returncode=p.returncode,reaped=p.returncode is not None,group_absent=probe() is False)
  e['streams']={}
  for name in ('stdout','stderr'):
   try:e['streams'][name]=pin(OUT/(label+'.'+name))
   except BaseException as error:e['errors'].append({'phase':'capture '+name,'error':str(error)})
  e['elapsed_seconds']=time.monotonic()-start
  e['passed']=not e['errors'] and e.get('returncode')==0 and e.get('reaped') is True and e.get('group_absent') is True and e['streams'].get('stderr',{}).get('bytes')==0
  save()
 assert e['passed'],label+' failed; first attempt retained without retry'
 return (OUT/(label+'.stdout')).read_bytes()
environment=dict(os.environ);environment.update(ASAN_OPTIONS='detect_leaks=0:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
try:
 assert call('mixed65-compile',argv,120,environment)==b''
 result['executable']=pin(OUT/'mixed65')
 raw=call('mixed65-run',[str(OUT/'mixed65')],30,environment)
 assert raw==oracle,'unchanged complete mixed65 oracle differs'
 result['actual_oracle']=raw.decode('ascii')
 result['status']='PASS_REMAINING_UNCHANGED65_ASAN_UBSAN_HOST_ONLY'
except BaseException as error:
 result.update(status='FIRST_COMPLETION_FAILURE_RETAINED_NO_RETRY',error=str(error) or type(error).__name__);raise
finally:
 try:
  result['selected_tree_after']=inventory(tree);result['repository_after']=inventory(REPO)
  result['proposal_after']={n:pin(PROPOSAL/n) for n in prior['proposal_before']}
  result['protected_after']={n:pin(REPO/n) for n in prior['protected_before']}
  result['first_retained_after']={n:pin(FIRST/n) for n in retained}
  result['compiler_after']=pin(Path('/usr/bin/cc'))
  assert result['selected_tree_after']==prior['selected_tree_before'] and result['repository_after']==prior['repository_before']
  assert result['proposal_after']==prior['proposal_before'] and result['protected_after']==prior['protected_before']
  assert result['first_retained_after']==retained and result['compiler_after']==prior['compiler_before']
 except BaseException as error:
  result.update(status='INPUT_CUSTODY_FAILURE',custody_error=str(error) or type(error).__name__);raise
 finally:save();print(OUT/'receipt.json')
