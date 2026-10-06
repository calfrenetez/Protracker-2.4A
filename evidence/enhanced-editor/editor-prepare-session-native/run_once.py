"""One exact portable software fixture; keep first failure resident, no automatic cleanup."""
import asyncio,datetime,fcntl,hashlib,json,os,stat,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;BASE=HERE.parent/'portable-v2'
from window_common import check_custody,check_complete,hold_failure,binding
from caller_guards import candidate_bytes,require_candidate_record
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
STAGE=INFRA/'runtime/Dev/Tests/PT-editor-prepare-session-20261006-v3'
os.environ['AMIGA_LAUNCH_LEASE']=str(HERE/'lease.private.json')
sys.path.insert(0,str(INFRA/'scripts'))
from launcher_access import operation,transport,install_client,install_ipc
from launcher_guard import LauncherGuard
from emulator import processes,owns
from shared_guest import Guest
MAX_LOG_BYTES=1048576
from bridge_checks import target,require_reply,checksum
from amiberry_mcp.ipc_client import AmiberryIPCClient
from mcp import ClientSession
from mcp.client.streamable_http import streamable_http_client

def fp(p):
 p=Path(p);s=p.lstat();assert stat.S_ISREG(s.st_mode) and not s.st_flags&0x40000000
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def save(name,value):
 with (HERE/name).open('x') as f:json.dump(value,f,indent=2);f.write('\n')
async def main():
 assert not (HERE/'run-terminal.json').exists()
 r={'status':'IN_PROGRESS','target':'amiberry-030','launches':0,'stage':str(STAGE),'atUTC':datetime.datetime.now(datetime.timezone.utc).isoformat(),'scope':'ordinary-memory software fixture with fake backend callbacks; no audio/device/nativeIRQ activation'}
 try:
  started=json.loads((HERE/'start-terminal.json').read_text());assert started['status']=='PASS_ONE_OWNED_030_START'
  product,pinned_count=check_custody()
  state=LauncherGuard(INFRA).status();assert state['phase']=='READY' and not state['inflight'] and not state['holds']
  with operation(INFRA,'amiberry-030','One exact editor preparation session and checked-lifetime fixture',may_leave_running=True):
   install_client();install_ipc(INFRA);guest=Guest(INFRA,HERE);client=AmiberryIPCClient(socket_path='/tmp/amiberry.sock')
   owned=processes();assert owned==[tuple(x) for x in started['ownedProcess']] and len(owned)==1 and owns(owned[0][1],'amiberry-030')
   def live():
    assert processes()==owned;target(INFRA,'amiberry-030')
   live();r['ownedProcess']=owned
   r['cpu']=await client.get_cpu_model();r['memory']=await client.get_memory_config();r['chipset']=await client.get_chipset();r['dmaBefore']=await client.get_dma_state();r['statusBefore']=guest.command('GET_STATUS')
   assert r['cpu']['model']=='68030' and str(r['cpu']['fpu'])=='0'
   assert r['memory']['chip']=='2048KB' and r['memory']['z3']=='131072KB' and r['memory']['rtg']=='0KB' and r['chipset'][1].upper()=='AGA'
   assert 'Paused=false' in r['statusBefore'] and all(r['dmaBefore']['audio'+str(i)]=='0' for i in range(4))
   immutable_candidate=dict(binding()['candidate'])
   binary=Path(product['path']).read_bytes();candidate_bytes(binary,immutable_candidate);dest='Dev:Tests/'+STAGE.name
   launcher=('FailAt 21\nStack 65536\nCD '+dest+'\nPTEditorPrepareSessionTest >fixture.log\nSet PTEditorResult $RC\nEcho $PTEditorResult >fixture.rc\nEcho COMPLETE >complete.flag\nQuit $PTEditorResult\n').encode('ascii')
   with transport(INFRA,'amiberry-030'):
    live();assert not STAGE.exists();STAGE.mkdir();(STAGE/'PTEditorPrepareSessionTest').open('xb').write(binary);(STAGE/'Run-once').open('xb').write(launcher)
   r['candidate']=fp(STAGE/'PTEditorPrepareSessionTest');require_candidate_record(r['candidate'],immutable_candidate);r['launcher']=fp(STAGE/'Run-once')
   async with streamable_http_client('http://127.0.0.1:3000/mcp') as (rd,wr,_):
    async with ClientSession(rd,wr) as session:
     await session.initialize()
     async def call(name,args):
      live();return require_reply(await asyncio.wait_for(session.call_tool(name,args),20))
     r['pingBefore']=await call('amiga_ping',{})
     checksum(await call('amiga_checksum',{'path':dest+'/PTEditorPrepareSessionTest'}),binary)
     checksum(await call('amiga_checksum',{'path':dest+'/Run-once'}),launcher)
     start=time.monotonic();deadline=start+420
     r['launches']=1
     r['launchReply']=await call('amiga_run_script',{'script':'Run >'+dest+'/launcher.log <NIL: Execute '+dest+'/Run-once','timeout':10})
     r['logObservations']=[];finished=False
     while time.monotonic()<deadline:
      live();log=STAGE/'fixture.log';data=b''
      if log.exists():
       assert log.is_file() and not log.is_symlink() and log.stat().st_size<=MAX_LOG_BYTES
       data=log.read_bytes()
      r['logObservations'].append({'bytes':len(data),'hostObservedSeconds':time.monotonic()-start})
      flag=STAGE/'complete.flag';rc=STAGE/'fixture.rc'
      if flag.exists() and rc.exists():
       assert not flag.is_symlink() and not rc.is_symlink() and flag.stat().st_size<=32 and rc.stat().st_size<=32
       assert flag.read_bytes()==b'COMPLETE\n' and rc.read_bytes()==b'0\n','First native nonzero or invalid completion retained'
       assert time.monotonic()<=deadline,'Absolute420second execution bound'
       r['completeAssertions']=check_complete(data);finished=True;break
      await asyncio.sleep(1)
     assert finished,'Exact420second observable fixture deadline; preserve resident failure'
     r['elapsedSeconds']=time.monotonic()-start
     r['downloadedLogs']={}
     for name in ('fixture.log','fixture.rc','complete.flag','launcher.log'):
      p=STAGE/name;assert p.is_file() and p.stat().st_size<=1048576
      if name!='launcher.log':assert p.stat().st_size>0
      b=p.read_bytes();(HERE/name).open('xb').write(b)
      checksum(await call('amiga_checksum',{'path':dest+'/'+name}),b)
      r['downloadedLogs'][name]=fp(HERE/name)
     assert (HERE/'fixture.rc').read_bytes().strip()==b'0' and (HERE/'complete.flag').read_bytes().strip()==b'COMPLETE'
     assert check_complete((HERE/'fixture.log').read_bytes())==r['completeAssertions']
     r['tasksAfter']=await call('amiga_run_script',{'script':'Status FULL','timeout':10})
     assert 'PTEditorPrepareSessionTest' not in r['tasksAfter'] and '[OK]' in r['tasksAfter']
     r['dmaAfter']=await client.get_dma_state();assert all(r['dmaAfter']['audio'+str(i)]=='0' for i in range(4))
     live();r['status']='PASS_EXACT_030_SOFTWARE_FIXTURE_SETTLEMENT_PENDING'
 except BaseException as e:
  r.update(status='FIRST_FAILURE_HOLD_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
  r['holdAttempt']=hold_failure()
  # Host custody only; do not send any additional target request or cleanup.
  for name in ('fixture.log','fixture.rc','complete.flag','launcher.log'):
   p=STAGE/name
   if p.is_file() and p.stat().st_size<=1048576 and not (HERE/('failure-'+name)).exists():(HERE/('failure-'+name)).write_bytes(p.read_bytes())
 finally:
  r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=datetime.datetime.now(datetime.timezone.utc).isoformat();save('run-terminal.json',r)
 print(json.dumps({k:r[k] for k in ('status','launches')}),flush=True)
 return 0 if r['status'].startswith('PASS_') else 20
with (INFRA/'runtime/test.lock').open('r+b') as f:
 fcntl.flock(f,fcntl.LOCK_EX|fcntl.LOCK_NB);sys.exit(asyncio.run(main()))
