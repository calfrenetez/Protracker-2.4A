"""Separate completed software-fixture settlement; no failed-run cleanup or forced kill."""
import asyncio,datetime,fcntl,hashlib,json,os,stat,sys,urllib.request
from pathlib import Path
HERE=Path(__file__).resolve().parent
from window_common import check_custody,check_complete,hold_failure,binding
from caller_guards import require_candidate_record
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
os.environ['AMIGA_LAUNCH_LEASE']=str(HERE/'lease.private.json');sys.path.insert(0,str(INFRA/'scripts'))
from launcher_access import operation,transport,install_client,install_ipc
from launcher_guard import LauncherGuard
from emulator import processes,owns,main as emulator_main
from shared_guest import Guest
from bridge_checks import require_reply,target
from amiberry_mcp.ipc_client import AmiberryIPCClient
from mcp import ClientSession
from mcp.client.streamable_http import streamable_http_client

def fp(p):
 p=Path(p);s=p.lstat();assert stat.S_ISREG(s.st_mode) and not stat.S_ISLNK(s.st_mode)
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def save(name,value):
 with (HERE/name).open('x') as f:json.dump(value,f,indent=2);f.write('\n')
async def main():
 assert not (HERE/'settlement-terminal.json').exists()
 r={'status':'IN_PROGRESS','atUTC':datetime.datetime.now(datetime.timezone.utc).isoformat(),'deletions':0,'forcedKills':0,'failureTargetRequests':0}
 try:
  run=json.loads((HERE/'run-terminal.json').read_text());assert run['status']=='PASS_EXACT_030_SOFTWARE_FIXTURE_SETTLEMENT_PENDING' and run['launches']==1
  require_candidate_record(run['candidate'],binding()['candidate'])
  check_custody();assert check_complete((HERE/'fixture.log').read_bytes())==run['completeAssertions']
  for name,h in run['downloadedLogs'].items():assert fp(HERE/name)==h
  state=LauncherGuard(INFRA).status();assert state['phase']=='READY' and not state['inflight'] and not state['holds']
  with operation(INFRA,'amiberry-030','Separate completed ProTracker memory fixture settlement',may_leave_running=True):
   install_client();install_ipc(INFRA);guest=Guest(INFRA,HERE);c=AmiberryIPCClient(socket_path='/tmp/amiberry.sock')
   owned=processes();assert owned==[tuple(x) for x in run['ownedProcess']] and len(owned)==1 and owns(owned[0][1],'amiberry-030')
   def live():assert processes()==owned;target(INFRA,'amiberry-030')
   async with streamable_http_client('http://127.0.0.1:3000/mcp') as (rd,wr,_):
    async with ClientSession(rd,wr) as session:
     await session.initialize();live()
     r['tasksAfter']=require_reply(await asyncio.wait_for(session.call_tool('amiga_run_script',{'script':'Status FULL','timeout':10}),20))
     assert '[OK]' in r['tasksAfter'] and 'PTEditorPrepareSessionTest' not in r['tasksAfter']
     live();r['cpu']=await c.get_cpu_model();r['memory']=await c.get_memory_config();r['chipset']=await c.get_chipset()
     assert r['cpu']['model']=='68030' and str(r['cpu']['fpu'])=='0'
     assert r['memory']['chip']=='2048KB' and r['memory']['z3']=='131072KB' and r['memory']['rtg']=='0KB' and r['chipset'][1].upper()=='AGA'
     r['dma']=await c.get_dma_state();r['statusBeforeClose']=guest.command('GET_STATUS')
     assert all(r['dma']['audio'+str(i)]=='0' for i in range(4)) and 'Paused=false' in r['statusBeforeClose']
     assert await asyncio.wait_for(c.screenshot(str(HERE/'os-before-settlement.png')),6)
     stage=Path(run['stage']);assert stage==INFRA/'runtime/Dev/Tests/PT-editor-prepare-session-20261006-v3'
     expected={'PTEditorPrepareSessionTest':run['candidate'],'Run-once':run['launcher'],**run['downloadedLogs']}
     assert len(expected)==6 and {p.name for p in stage.iterdir()}==set(expected)
     for name,h in expected.items():assert fp(stage/name)==h
     custody=HERE/'stage-custody';assert not custody.exists()
     with transport(INFRA,'amiberry-030'):
      live()
      for name,h in expected.items():assert fp(stage/name)==h
      os.rename(stage,custody)
     assert not stage.exists()
     for name,h in expected.items():assert fp(custody/name)==h
     r['custody']={'original':str(stage),'directory':str(custody),'files':expected,'stageAbsent':True}
     live();r['disconnect']=require_reply(await asyncio.wait_for(session.call_tool('amiga_disconnect',{}),10))
     with urllib.request.urlopen('http://127.0.0.1:3000/health',timeout=3) as response:health=json.load(response)
     assert health['serial']['connected'] is False;r['healthAfterDisconnect']=health
   assert processes()==owned
   await emulator_main(['stop','--target','amiberry-030'])
   assert not processes();r['ownedProcessClosed']=owned
  r['status']='PASS_COMPLETED_FIXTURE_SETTLEMENT_RELEASE_PENDING'
 except BaseException as e:
  r.update(status='FIRST_SETTLEMENT_FAILURE_HOLD_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
  r['holdAttempt']=hold_failure()
 finally:
  r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=datetime.datetime.now(datetime.timezone.utc).isoformat();save('settlement-terminal.json',r)
 print(json.dumps({'status':r['status'],'deletions':0,'forcedKills':0}),flush=True)
 return 0 if r['status'].startswith('PASS_') else 20
with (INFRA/'runtime/test.lock').open('r+b') as f:
 fcntl.flock(f,fcntl.LOCK_EX|fcntl.LOCK_NB);sys.exit(asyncio.run(main()))
