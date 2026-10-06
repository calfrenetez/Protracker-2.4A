"""Separate independent failed-start recovery readback, then exact normal release."""
import datetime,fcntl,hashlib,json,os,stat,subprocess,sys,urllib.request
from pathlib import Path
HERE=Path(__file__).resolve().parent;INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
os.environ['AMIGA_LAUNCH_LEASE']=str(HERE/'lease.private.json');sys.path.insert(0,str(INFRA/'scripts'))
from emulator import processes
from launcher_guard import LauncherGuard
from launcher_access import lease_input

def fp(p):
 p=Path(p);s=p.lstat();assert stat.S_ISREG(s.st_mode) and not stat.S_ISLNK(s.st_mode) and not s.st_flags&0x40000000
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def save(name,value):
 with (HERE/name).open('x') as f:json.dump(value,f,indent=2);f.write('\n')
assert not (HERE/'startup-root-verified-release.json').exists()
closed=json.loads((HERE/'startup-recovery-terminal.json').read_text());approval=json.loads((HERE/'explicit-startup-recovery-approval.json').read_text())
assert approval['approved'] is True and approval['pid']==35970 and approval['fence']==40
assert closed['status']=='PASS_FAILED_START_RECOVERY_HOLD_RETAINED' and closed['sigtermCalls']==1 and not closed['forcedKills'] and not closed['guestRequests'] and not closed['deletions'] and not closed['releaseCalls']
locks=[]
try:
 for name in ('test.lock','real-a1200-safari.lock'):
  fd=os.open(INFRA/'runtime'/name,os.O_RDWR|os.O_NOFOLLOW);fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);locks.append(fd)
 guard=LauncherGuard(INFRA);before=guard.status();assert before==closed['guardBefore']==closed['guardAfter'] and before['phase']=='HOLD' and before['fence']==40 and not before['inflight']
 lease=lease_input(INFRA,'amiberry-030');assert before['reservation']['lease']['run_id']==lease['run_id']
 assert not processes()
 p=subprocess.run(['/usr/sbin/lsof','-nP','-iTCP:2345','-sTCP:LISTEN'],capture_output=True,timeout=5);assert p.returncode==1 and not p.stdout
 with urllib.request.urlopen('http://127.0.0.1:3000/health',timeout=3) as response:health=json.load(response)
 assert health['status']=='ok' and health['serial']['connected'] is False and (health['serial']['host'],health['serial']['port'])==('127.0.0.1',2345)
 pins=json.loads((HERE/'pins.json').read_text())['files']
 for p,h in pins.items():assert fp(p)==h
 for n,h in closed['originalRecords'].items():assert fp(HERE/n)==h
 failed=json.loads((HERE/'start-terminal.json').read_text());assert fp(INFRA/'runtime/Dev/Tools/bridge-enabled')==failed['bridgeFlagBefore']
 assert not (INFRA/'runtime/Dev/Tests/PT-master-progress-20261005-v1').exists() and not (HERE/'run-terminal.json').exists()
 records={n:fp(HERE/('startup-recovery-'+n+'.json'))['sha256'] for n in ('authorization','restoration','absence','cleanup')}
 proof={'run_id':lease['run_id'],'target':'amiberry-030','fence':before['fence'],'externally_verified':True,'records':records,'facts':{n:True for n in records}}
 save('startup-release-proof.json',proof)
 assert not processes() and guard.status()==before
 result=guard.release(lease,proof);after=guard.status();assert after['phase']=='EMPTY' and after['fence']==41 and not after['reservation'] and not after['holds'] and not after['inflight'] and not after['potential_resident']
 save('startup-root-verified-release.json',{'status':'PASS_EXACT_FAILED_START_RECOVERY_RELEASE','release':result,'guardAfter':after,'records':records,'independentReadback':{'allEmulatorsAbsent':True,'listenerAbsent':True,'DevBenchDisconnected':True,'pinnedInputsUnchanged':len(pins),'allFirstRecordsPreserved':True,'candidateNeverStagedOrLaunched':True},'physical':'NOT_RUN','atUTC':datetime.datetime.now(datetime.timezone.utc).isoformat()})
 print(json.dumps(result))
finally:
 for fd in reversed(locks):os.close(fd)
