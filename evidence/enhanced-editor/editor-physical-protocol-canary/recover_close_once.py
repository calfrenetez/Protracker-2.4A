"""Prepared exact recovery only; separate human approval required. No guest call."""
from window_common import *
import signal,time
assert not (HERE/'recovery-terminal.json').exists()
r={'status':'PREPARING','sigtermCalls':0,'forcedKills':0,'guestRequests':0,'deletions':0,'releaseCalls':0,'physicalTouched':False,'atUTC':now()}
try:
 approval=load('explicit-recovery-approval.json');assert approval['approved'] is True and approval['pid']==93129 and approval['fence']==85 and approval['plan']==fp(HERE/'recovery-plan.json')
 coord=load('recovery-coordination.json');assert coord['allPeersHandsOff'] is True and set(coord['peers'])=={'Scott','AmiConnect','AmiGUS'}
 with locks():
  fd=os.open(INFRA/'runtime/launcher-transport.lock',os.O_RDWR|os.O_NOFOLLOW)
  try:
   fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);custody();peer_gate()
   for p,h in load('recovery-pins.json')['files'].items():assert fp(p)==h,p
   failed=load('settlement-terminal.json');identity=load('host-only-failure-identity-v1.json');run=load('run-terminal.json');started=load('start-terminal.json')
   guard=LauncherGuard(INFRA);before=guard.status();assert before==failed['guardAfter']==identity['guard'] and before['phase']=='HOLD' and before['fence']==85 and before['inflight'] is None
   assert failed['errorType']=='TypeError' and failed['error']=="'types.SimpleNamespace' object is not iterable"
   assert failed['cleanup']=='[OK]\nPT-CLEAN' and failed['independentAbsence']=='[OK]\nPT-ABSENT' and failed['calls'][-1]['tool']=='amiga_disconnect' and failed['calls'][-1]['response']=='Disconnected'
   assert all(failed['dmaAfter']['audio'+str(i)]=='0' for i in range(4)) and 'Paused=false' in failed['statusAfter']
   os.environ['AMIGA_LAUNCH_LEASE']=str(HERE/'return.lease.private.json');lease=lease_input(INFRA,'amiberry-030');assert before['reservation']['lease']['run_id']==lease['run_id']
   rows=processes();assert rows==[tuple(x) for x in identity['ownedProcess']] and len(rows)==1 and rows[0][0]==93129 and owns(rows[0][1],'amiberry-030')
   assert json.loads((INFRA/'runtime/emulator.json').read_text())==identity['runtime']
   assert health()==started['originalEndpoint']==identity['originalEndpoint'] and fp(INFRA/'runtime/Dev/Tools/bridge-enabled')==started['originalBridgeFlag']
   directory=HERE/'canary-custody';assert {p.name for p in directory.iterdir()}==set(protocol.FILES)
   for n,h in run['files'].items():assert fp(directory/n)==h,n
   r.update(guardBefore=before,ownedProcess=rows,inputs=load('recovery-pins.json')['files']);save('recovery-preflight.json',r)
   assert processes()==rows and guard.status()==before
   r['sigtermCalls']=1;os.kill(93129,signal.SIGTERM);end=time.monotonic()+5
   while processes() and time.monotonic()<end:time.sleep(.1)
   absent_emulator();assert health()==identity['originalEndpoint']
   custody();assert guard.status()==before and fp(INFRA/'runtime/Dev/Tools/bridge-enabled')==identity['bridgeFlag']
   for p,h in load('recovery-pins.json')['files'].items():assert fp(p)==h,p
   r.update(status='PASS_EXACT_CANARY_RECOVERY_CLOSURE_HOLD_RETAINED',emulatorAbsent=True,listenerAbsent=True,DevBenchDisconnectedOriginalEndpoint=True)
  finally:os.close(fd)
except BaseException as e:r.update(status='FIRST_RECOVERY_FAILURE_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
finally:r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=now();save('recovery-terminal.json',r)
print(r['status'])
if r['status']!='PASS_EXACT_CANARY_RECOVERY_CLOSURE_HOLD_RETAINED':raise SystemExit(20)
