"""Independent readback after approved exact recovery, then ordinary four-record release."""
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
assert not (HERE/'root-verified-release.json').exists()
approval=json.loads((HERE/'explicit-recovery-approval.json').read_text());assert approval['approved'] is True and approval['pid']==32716 and approval['fence']==33
closed=json.loads((HERE/'recovery-close-terminal.json').read_text());assert closed['status']=='PASS_OWNED_EMULATOR_RECOVERY_HOLD_RETAINED' and closed['sigtermCalls']==1 and not closed['forcedKills'] and not closed['guestRequests'] and not closed['guardReleaseCalls']
locks=[]
try:
 for name in ('test.lock','real-a1200-safari.lock'):
  fd=os.open(INFRA/'runtime'/name,os.O_RDWR|os.O_NOFOLLOW);fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);locks.append(fd)
 guard=LauncherGuard(INFRA);before=guard.status();assert before==closed['guardBefore']==closed['guardAfter'] and before['phase']=='HOLD' and before['fence']==33 and before['inflight'] is None
 lease=lease_input(INFRA,'amiberry-030');assert before['reservation']['lease']['run_id']==lease['run_id']
 assert not processes()
 p=subprocess.run(['/usr/sbin/lsof','-nP','-iTCP:2345','-sTCP:LISTEN'],capture_output=True,timeout=5);assert p.returncode==1 and not p.stdout
 with urllib.request.urlopen('http://127.0.0.1:3000/health',timeout=3) as response:health=json.load(response)
 assert health['status']=='ok' and health['serial']['connected'] is False and (health['serial']['host'],health['serial']['port'])==('127.0.0.1',2345)
 pins=json.loads((HERE/'pins.json').read_text())['files']
 for path,h in pins.items():assert fp(path)==h
 assert (INFRA/'runtime/Dev/Tools/bridge-enabled').read_bytes()==b''
 custody=closed['custody'];assert not Path(custody['original']).exists()
 path=Path(custody['directory']);assert path==HERE/'failed-stage-custody' and {p.name for p in path.iterdir()}==set(custody['files'])
 for name,h in custody['files'].items():assert fp(path/name)==h
 assert fp(HERE/'run-terminal.json')==closed['originalFailedResult']
 records={name:fp(HERE/('recovery-'+name+'.json'))['sha256'] for name in ('authorization','restoration','absence','cleanup')}
 proof={'run_id':lease['run_id'],'target':'amiberry-030','fence':before['fence'],'externally_verified':True,'records':records,'facts':{name:True for name in records}}
 save('release-proof.json',proof)
 assert not processes() and guard.status()==before
 release=guard.release(lease,proof);after=guard.status();assert after['phase']=='EMPTY' and after['fence']==34 and not after['inflight'] and not after['reservation'] and not after['holds'] and not after['potential_resident']
 save('root-verified-release.json',{'status':'PASS_EXACT_OWNED_RECOVERY_VERIFIED_RELEASE','release':release,'guardAfter':after,'records':records,'independentReadback':{'allEmulatorsAbsent':True,'port2345ListenerAbsent':True,'DevBenchDisconnected':True,'exactStageAbsentAllCustodyBytesPreserved':True,'originalInputsUnchanged':len(pins)},'originalNativeAttempt':'FAILED, not retried or promoted','physical':'NOT_RUN','atUTC':datetime.datetime.now(datetime.timezone.utc).isoformat()})
 print(json.dumps(release))
finally:
 for fd in reversed(locks):os.close(fd)
