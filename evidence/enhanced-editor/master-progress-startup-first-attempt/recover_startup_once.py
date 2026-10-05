"""Prepared separately approved exact failed-start closure. Never retry a target."""
import datetime,fcntl,hashlib,json,os,signal,stat,subprocess,sys,time,urllib.request
from pathlib import Path
HERE=Path(__file__).resolve().parent;INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
os.environ['AMIGA_LAUNCH_LEASE']=str(HERE/'lease.private.json');sys.path.insert(0,str(INFRA/'scripts'))
from emulator import processes,owns
from launcher_guard import LauncherGuard
from launcher_access import lease_input

def fp(p):
 p=Path(p);s=p.lstat();assert stat.S_ISREG(s.st_mode) and not stat.S_ISLNK(s.st_mode) and not s.st_flags&0x40000000
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def save(name,value):
 with (HERE/name).open('x') as f:json.dump(value,f,indent=2);f.write('\n')
assert not (HERE/'startup-recovery-terminal.json').exists()
r={'status':'IN_PROGRESS','sigtermCalls':0,'forcedKills':0,'guestRequests':0,'deletions':0,'releaseCalls':0,'atUTC':datetime.datetime.now(datetime.timezone.utc).isoformat()};locks=[]
try:
 approval=json.loads((HERE/'explicit-startup-recovery-approval.json').read_text());assert approval['approved'] is True and approval['pid']==35970 and approval['fence']==40 and approval['scope']=='one SIGTERM of own failed-start emulator, independent verified release; no candidate retry or physical action'
 coord=json.loads((HERE/'startup-recovery-coordination.json').read_text());assert coord['approved_scope'] is True
 failed=json.loads((HERE/'start-terminal.json').read_text());identity=json.loads((HERE/'host-only-failure-identity-v1.json').read_text())
 assert failed['status']=='FIRST_FAILURE_RETAINED_NO_RETRY' and failed['startCalls']==failed['reserveCalls']==1
 pins=json.loads((HERE/'pins.json').read_text())['files']
 for p,h in pins.items():assert fp(p)==h
 for name in ('test.lock','real-a1200-safari.lock','launcher-transport.lock'):
  fd=os.open(INFRA/'runtime'/name,os.O_RDWR|os.O_NOFOLLOW);fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);locks.append(fd)
 guard=LauncherGuard(INFRA);before=guard.status();assert before==failed['guardAfter'] and before['phase']=='HOLD' and before['fence']==40 and before['inflight'] is None
 lease=lease_input(INFRA,'amiberry-030');assert before['reservation']['lease']['run_id']==lease['run_id']
 rows=processes();assert rows==[tuple(x) for x in identity['ownedProcess']] and len(rows)==1 and rows[0][0]==35970 and owns(rows[0][1],'amiberry-030')
 assert json.loads((INFRA/'runtime/emulator.json').read_text())==identity['runtime']
 stage=INFRA/'runtime/Dev/Tests/PT-master-progress-20261005-v1';assert not stage.exists() and not (HERE/'run-terminal.json').exists()
 original={n:fp(HERE/n) for n in ('start-terminal.json','start.stdout','start.stderr','preflight.json','host-only-failure-identity-v1.json')}
 r.update(guardBefore=before,ownedProcess=rows[0],originalRecords=original)
 save('startup-recovery-preflight.json',r)
 assert processes()==rows and guard.status()==before
 r['sigtermCalls']=1;os.kill(35970,signal.SIGTERM)
 end=time.monotonic()+5
 while processes() and time.monotonic()<end:time.sleep(.1)
 assert not processes(),'Owned emulator did not close; no force kill'
 p=subprocess.run(['/usr/sbin/lsof','-nP','-iTCP:2345','-sTCP:LISTEN'],capture_output=True,timeout=5);assert p.returncode==1 and not p.stdout
 with urllib.request.urlopen('http://127.0.0.1:3000/health',timeout=3) as response:health=json.load(response)
 assert health['status']=='ok' and health['serial']['connected'] is False and (health['serial']['host'],health['serial']['port'])==('127.0.0.1',2345)
 for p,h in pins.items():assert fp(p)==h
 for n,h in original.items():assert fp(HERE/n)==h
 assert not stage.exists() and not (HERE/'run-terminal.json').exists() and guard.status()==before
 assert fp(INFRA/'runtime/Dev/Tools/bridge-enabled')==failed['bridgeFlagBefore']
 save('startup-recovery-authorization.json',{'directApproval':fp(HERE/'explicit-startup-recovery-approval.json'),'coordination':fp(HERE/'startup-recovery-coordination.json'),'scope':'one exact owned failed-start closure and independent normal release only'})
 save('startup-recovery-restoration.json',{'pinnedInputsUnchanged':len(pins),'DevBenchDisconnectedOriginalEndpoint':True,'bridgeFlagUnchanged':True,'serviceRestartReset':False})
 save('startup-recovery-absence.json',{'exact35970AndAllEmulatorsAbsent':True,'2345ListenerAbsent':True,'DevBenchDisconnected':True,'candidateNeverStagedOrLaunched':True,'physicalTouched':False})
 save('startup-recovery-cleanup.json',{'sigtermCalls':1,'forcedKills':0,'deletions':0,'allFirstRecordsPreserved':original,'candidateRetry':False,'stageAbsentNoCleanupRequired':True,'guardStillHeld':True})
 r.update(status='PASS_FAILED_START_RECOVERY_HOLD_RETAINED',healthAfter=health,emulatorAbsent=True,listenerAbsent=True,stageAbsent=True)
except BaseException as e:r.update(status='FIRST_RECOVERY_FAILURE_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
finally:
 for fd in reversed(locks):os.close(fd)
 r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=datetime.datetime.now(datetime.timezone.utc).isoformat();save('startup-recovery-terminal.json',r)
print(json.dumps({k:r[k] for k in ('status','sigtermCalls','forcedKills','guestRequests','deletions')}))
if r['status']!='PASS_FAILED_START_RECOVERY_HOLD_RETAINED':raise SystemExit(20)
