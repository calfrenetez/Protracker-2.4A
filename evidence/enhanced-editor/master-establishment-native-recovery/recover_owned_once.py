"""Separate bounded owned-emulator recovery; preserve first failure, no guest exchange or retry."""
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
assert not (HERE/'recovery-close-terminal.json').exists()
r={'status':'IN_PROGRESS','sigtermCalls':0,'forcedKills':0,'guestRequests':0,'deletions':0,'guardReleaseCalls':0,'atUTC':datetime.datetime.now(datetime.timezone.utc).isoformat()};locks=[]
try:
 approval=json.loads((HERE/'explicit-recovery-approval.json').read_text())
 assert approval['approved'] is True and approval['pid']==32716 and approval['fence']==33 and approval['scope']=='one SIGTERM, exact failed-stage custody, independent verified release; no candidate rerun or physical action'
 assert json.loads((HERE/'recovery-coordination.json').read_text())['revision']>=14
 pins=json.loads((HERE/'pins.json').read_text())['files']
 for p,h in pins.items():assert fp(p)==h
 failed=json.loads((HERE/'run-terminal.json').read_text());assert failed['status']=='FIRST_FAILURE_HOLD_NO_RETRY' and failed['launches']==1
 for name in ('test.lock','real-a1200-safari.lock','launcher-transport.lock'):
  fd=os.open(INFRA/'runtime'/name,os.O_RDWR|os.O_NOFOLLOW);fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);locks.append(fd)
 guard=LauncherGuard(INFRA);before=guard.status();assert before==failed['guardAfter'] and before['phase']=='HOLD' and before['fence']==33 and before['inflight'] is None
 lease=lease_input(INFRA,'amiberry-030');assert before['reservation']['lease']['run_id']==lease['run_id']
 rows=processes();assert rows==[tuple(x) for x in failed['ownedProcess']] and len(rows)==1 and rows[0][0]==32716 and owns(rows[0][1],'amiberry-030')
 runtime=json.loads((INFRA/'runtime/emulator.json').read_text());assert runtime['pid']==32716 and runtime['target']=='amiberry-030'
 stage=Path(failed['stage']);assert stage==INFRA/'runtime/Dev/Tests/PT-master-establish-20261005-v1'
 assert fp(stage/'PTMasterEstablishTest')==failed['candidate'] and fp(stage/'Run-once')==failed['launcher']
 allowed={'PTMasterEstablishTest','Run-once','fixture.log','fixture.rc','complete.flag','launcher.log'};assert {p.name for p in stage.iterdir()}<=allowed
 save('recovery-authorization.json',{'humanSource':approval['human_response'], 'explicitApproval':fp(HERE/'explicit-recovery-approval.json'),'scope':'Separate reversible closure of only this fresh root-owned emulator, exact failed-stage custody, independent absence/restoration and ordinary bound release; no app rerun, physical/audio action or forced kill','plan':fp(HERE/'recovery-plan.json'),'coordination':fp(HERE/'recovery-coordination.json')})
 r.update(guardBefore=before,ownedProcess=rows[0],originalFailedResult=fp(HERE/'run-terminal.json'))
 save('recovery-close-preflight.json',r)
 assert processes()==rows and guard.status()==before
 r['sigtermCalls']=1;os.kill(32716,signal.SIGTERM)
 end=time.monotonic()+5
 while processes() and time.monotonic()<end:time.sleep(.1)
 assert not processes(),'Owned emulator did not close: retain HOLD, no force kill'
 p=subprocess.run(['/usr/sbin/lsof','-nP','-iTCP:2345','-sTCP:LISTEN'],capture_output=True,timeout=5);assert p.returncode==1 and not p.stdout
 with urllib.request.urlopen('http://127.0.0.1:3000/health',timeout=3) as response:health=json.load(response)
 assert health['status']=='ok' and health['serial']['connected'] is False
 assert (health['serial']['host'],health['serial']['port'])==('127.0.0.1',2345)
 assert guard.status()==before
 # Guest and all its virtual readers are absent before exact host custody move.
 expected={p.name:fp(p) for p in stage.iterdir()};assert set(expected)<=allowed
 assert expected['PTMasterEstablishTest']==failed['candidate'] and expected['Run-once']==failed['launcher']
 custody=HERE/'failed-stage-custody';assert not custody.exists()
 assert not processes() and guard.status()==before
 os.rename(stage,custody);assert not stage.exists()
 assert {p.name for p in custody.iterdir()}==set(expected)
 for name,h in expected.items():assert fp(custody/name)==h
 for path,h in pins.items():assert fp(path)==h
 assert (INFRA/'runtime/Dev/Tools/bridge-enabled').read_bytes()==b'' and guard.status()==before
 r.update(status='PASS_OWNED_EMULATOR_RECOVERY_HOLD_RETAINED',healthAfter=health,emulatorAbsent=True,listenerAbsent=True,custody={'original':str(stage),'directory':str(custody),'files':expected})
 save('recovery-restoration.json',{'pinnedInputsUnchanged':len(pins),'profileUnchanged':True,'DevBenchDisconnected':True,'bridgeFlagBytesUnchanged':True,'serviceRestart':False,'guardReset':False,'diskRollback':False})
 save('recovery-absence.json',{'exactOwnedEmulator32716Absent':True,'allEmulatorsAbsent':True,'port2345ListenerAbsent':True,'DevBenchDisconnected':True,'virtualGuestAndItsReadersGoneWithOwnedProcess':True,'physicalGuestTouched':False})
 save('recovery-cleanup.json',{'sigtermCalls':1,'forcedKills':0,'deletions':0,'failedStageCustody':r['custody'],'firstFailureAndHostSnapshotsPreserved':True,'candidateRetried':False,'guardStillHeld':True,'qualification':'Original attempt remains FAILED, not a late or native pass'})
except BaseException as e:r.update(status='FIRST_RECOVERY_FAILURE_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
finally:
 for fd in reversed(locks):os.close(fd)
 r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=datetime.datetime.now(datetime.timezone.utc).isoformat();save('recovery-close-terminal.json',r)
print(json.dumps({k:r[k] for k in ('status','sigtermCalls','forcedKills','guestRequests','deletions')}))
if r['status']!='PASS_OWNED_EMULATOR_RECOVERY_HOLD_RETAINED':raise SystemExit(20)
