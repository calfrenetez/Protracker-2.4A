"""Root isolated one-attempt HOST CRC and unchanged mixed65 qualification."""
import ast,hashlib,json,os,re,shutil,signal,subprocess,time
from pathlib import Path
BASE=Path(__file__).resolve().parent
REPO=Path('/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A')
PROPOSAL=BASE/'native-project-crc-nibble-source-proposal-v1'
OUT=BASE/'root-project-crc-nibble-host-first-v1'
EXPECTED={'src/core/project.c':(47003,'f1bad97dc044e3976936be95c4e42051e8b7a79aab643a1c984a14a1dd24c75a'),'tests/project_crc_nibble_test.c':(8146,'119ef69ea5d8a78f5fc13a0beae798d18a85cc164bc43adaea1c0cc0142affb3'),'tests/test_project_crc_nibble.py':(3413,'4664632fd002f9d196df6023ce003093ceb3ab967289c2c10e53f1dacd0a304e'),'PLAN.md':(7188,'111b01809bb91ca3f044a9e683415daadeaa5244d48c7ea851f1ab1aa5f12614'),'project.c.diff':(2796,'695c25d6afef5995d8da246f804ad6279228590c805af5ac0d763a380608d9d0')}
def pin(p):
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'mode':p.stat().st_mode}
def inventory(root):
 paths=[p for d in ('src','tests') for p in (root/d).rglob('*') if p.is_file()]
 assert len(paths)<=2048 and all(not p.is_symlink() and p.stat().st_size<=16777216 for p in paths)
 return {str(p.relative_to(root)):pin(p) for p in paths}
protected=json.loads((BASE/'root-native-mixed-ram-fast-chip-build-first-v2/receipt.json').read_text())['protected_before']
assert all(pin(REPO/n)==v for n,v in protected.items())
proposal={n:pin(PROPOSAL/n) for n in EXPECTED}
assert all((p['bytes'],p['sha256'],p['mode'])==(*EXPECTED[n],33188) for n,p in proposal.items())
review=BASE/'independent-project-crc-nibble-source-v1-review.json'
assert pin(review)=={'bytes':7376,'sha256':'2004464b0678e6180902fb5b78f304215749ecb034ff50a3d983a68784f68cf2','mode':33188}
assert json.loads(review.read_text())['passed'] is True
repo_before=inventory(REPO);OUT.mkdir();tree=OUT/'tree'
for d in ('src','tests'):shutil.copytree(REPO/d,tree/d)
for n in ('src/core/project.c','tests/project_crc_nibble_test.c','tests/test_project_crc_nibble.py'):shutil.copy2(PROPOSAL/n,tree/n)
before=inventory(tree)
old=BASE/'root-mixed-ram-diagnostic-host-first-v1/receipt.json'
assert pin(old)=={'bytes':311986,'sha256':'78753bcd71b26d66759fe09f951dbefa10b8b4b7cf41ebbaf965d92fff5b35be','mode':33188}
mixed_oracle=json.loads(old.read_text())['actual_oracle'].encode('ascii')
sources=None
for node in ast.parse((tree/'tests/test_native_mixed_ram_port.py').read_text()).body:
 if isinstance(node,ast.Assign) and len(node.targets)==1 and isinstance(node.targets[0],ast.Name) and node.targets[0].id=='SOURCES':sources=ast.literal_eval(node.value)
assert sources and len(sources)==28
result={'scope':'HOST_ONLY_CRC_COMPATIBILITY_AND_UNCHANGED_MIXED65','status':'NOT_COMPLETE','source_review':pin(review),'repository_before':repo_before,'selected_tree_before':before,'proposal_before':proposal,'protected_before':protected,'commands':[],'target_operation':False,'automatic_retry':False}
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
flags=['-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-UNDEBUG','-fsanitize=address,undefined','-Isrc/core','-I.']
core=['src/core/project.c','src/core/channels.c','src/core/pcm.c']
goldens={n:pin(tree/'tests/fixtures/project-v1'/n) for n in ('mixed.ptg','song.ptg')}
crc_oracle=b'PROJECT CRC NIBBLE PASS: independent bitwise transitions/all 256 bytes/state basis; split/hole/read failures; mixed/song fixed goldens, exact encode/stream/positional decode; no reduced validation\n'
project_oracle=b'PROJECT PASS: exact mixed-route round trip, 24-bit stereo, OFF, loops, slices, MIDI endpoints, extensions, CRC, bounds, transactional decode\n'
reader_oracle=b'PROJECT READER PASS: exact master re-encode, decoder failure staging, bounded reads, original parser parity, truncation, CRC and repaired mutations, every read failure leaves requirements unchanged\n'
groups=[('focused',['tests/project_crc_nibble_test.c','src/core/channels.c','src/core/pcm.c'],[tree/'tests/fixtures/project-v1/mixed.ptg',tree/'tests/fixtures/project-v1/song.ptg'],crc_oracle),('project',['tests/project_test.c',*core],[OUT/'mixed.ptg'],project_oracle),('stream',['tests/project_stream_test.c',*core,'src/platform/project_file.c','src/platform/file_save.c','src/core/safe_save.c'],[OUT/'master.ptg'],None),('stream-faults',['-Dread=pt_test_read','tests/render_read_faults.c','tests/project_stream_test.c',*core,'src/platform/project_file.c','src/platform/file_save.c','src/core/safe_save.c'],[OUT/'master-faults.ptg'],None),('reader',['tests/project_reader_test.c',*core],[tree/'tests/fixtures/project-v1/mixed.ptg'],reader_oracle),('mixed65',sources,[],mixed_oracle)]
try:
 result['compiler_before']=pin(Path('/usr/bin/cc'));result['goldens_before']=goldens;result['oracles']={};result['executables']={}
 for label,units,args,oracle in groups:
  executable=OUT/label
  assert call(label+'-compile',['/usr/bin/cc',*flags,*units,'-o',str(executable)],120,environment)==b''
  result['executables'][label]=pin(executable)
  raw=call(label+'-run',[str(executable),*map(str,args)],30,environment)
  if oracle is None:assert re.fullmatch(rb'PROJECT STREAM PASS: exact golden layout/CRC, mixed masters/loops/slices/extensions, sink refusal, bounded workspace=[1-9][0-9]*\n',raw)
  else:assert raw==oracle,label+' complete oracle differs'
  result['oracles'][label]=raw.decode('ascii')
  if label=='project':assert (OUT/'mixed.ptg').read_bytes()==(tree/'tests/fixtures/project-v1/mixed.ptg').read_bytes()
  save()
 result['goldens_after']={n:pin(tree/'tests/fixtures/project-v1'/n) for n in goldens};assert result['goldens_after']==goldens
 result['selected_tree_after']=inventory(tree);result['repository_after']=inventory(REPO);result['proposal_after']={n:pin(PROPOSAL/n) for n in EXPECTED};result['protected_after']={n:pin(REPO/n) for n in protected}
 assert result['selected_tree_after']==before and result['repository_after']==repo_before and result['proposal_after']==proposal and result['protected_after']==protected and pin(Path('/usr/bin/cc'))==result['compiler_before']
 result['status']='PASS_5_CRC_GROUPS_AND_UNCHANGED65_ASAN_UBSAN_HOST_ONLY'
except BaseException as error:
 result.update(status='FIRST_FAILURE_RETAINED_NO_RETRY',error=str(error) or type(error).__name__);raise
finally:save();print(OUT/'receipt.json')
