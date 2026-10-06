"""One fresh owned030 lifecycle; no retries, resets, service changes or recovery."""
import datetime,fcntl,hashlib,json,os,stat,subprocess,sys,urllib.request
from pathlib import Path
HERE=Path(__file__).resolve().parent;BASE=HERE.parent/'portable-v2'
from window_common import check_custody,binding
from caller_guards import hold_start_failure
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
sys.path.insert(0,str(INFRA/'scripts'))
from launcher_guard import LauncherGuard,_pack
from emulator import processes

def fp(p):
 p=Path(p);s=p.lstat();assert stat.S_ISREG(s.st_mode) and not s.st_flags&0x40000000,str(p)
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def save(name,value):
 with (HERE/name).open('x') as f:json.dump(value,f,indent=2);f.write('\n')
assert not (HERE/'start-terminal.json').exists()
result={'status':'PREPARING','reserveCalls':0,'startCalls':0,'atUTC':datetime.datetime.now(datetime.timezone.utc).isoformat(),'target':'amiberry-030','scope':'one exact ordinary-memory qualification; no physical/audio/device activation'}
local_lease=None;local_guard=None;reservation_binding=None
controller='ProTracker root01a0b5ae-c04f-7661-8859-e89df20c3290'
try:
 product,pinned_count=check_custody()
 coordination=json.loads((HERE/'coordination.json').read_text())
 assert coordination['status']=='FRESH_ROOT_SCOPE_ALL_PEERS_HANDS_OFF'
 assert set(coordination['peers'])=={'AmiConnect','Scott','AmiGUS'}
 assert all(v['admissionConfirmed'] is True for v in coordination['peers'].values())
 assert json.loads((HERE/'preexecution-review.json').read_text())['status']=='PASS_ROOT_SELF_PREEXECUTION_REVIEW'
 prior=json.loads((HERE/'prior-release.json').read_text())
 assert fp(prior['path'])==prior['fingerprint']
 guard=LauncherGuard(INFRA);local_guard=guard;state=guard.status()
 assert state['phase']=='EMPTY' and not state['reservation'] and not state['inflight'] and not state['holds'] and not state['potential_resident']
 assert state==prior['guardAfter']
 assert not processes(),'Any existing emulator refuses admission'
 p=subprocess.run(['/usr/sbin/lsof','-nP','-iTCP:2345','-sTCP:LISTEN'],capture_output=True,timeout=5)
 assert p.returncode==1 and not p.stdout and not p.stderr
 with urllib.request.urlopen('http://127.0.0.1:3000/health',timeout=3) as r:health=json.load(r)
 assert health['status']=='ok' and health['serial']['connected'] is False and (health['serial']['host'],health['serial']['port'])==('127.0.0.1',2345)
 fds=[]
 try:
  for name in ('test.lock','real-a1200-safari.lock','launcher-transport.lock'):
   fd=os.open(INFRA/'runtime'/name,os.O_RDWR|os.O_NOFOLLOW);fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);fds.append(fd)
  assert guard.status()==state and not processes()
 finally:
  for fd in reversed(fds):os.close(fd)
 flag=INFRA/'runtime/Dev/Tools/bridge-enabled';assert flag.is_file() and flag.read_bytes()==b''
 result.update(guardBefore=state,healthBefore=health,bridgeFlagBefore=fp(flag),candidate=product,pinnedFiles=pinned_count)
 reservation_binding={'candidateSHA256':product['sha256'],'candidateBytes':product['bytes'],'sourceCommit':binding()['sourceCommit'],'scopeSHA256':fp(HERE/'scope-plan.json')['sha256'],'coordinationSHA256':fp(HERE/'coordination.json')['sha256'],'evidence':str(HERE)}
 fd=os.open(HERE/'lease.private.json',os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 try:
  result['reserveCalls']=1
  local_lease=guard.reserve('amiberry-030',controller,'Human authorized exact portable editor preparation session and checked-lifetime qualification; fresh sole-controller scope',reservation_binding)
  with os.fdopen(fd,'wb') as f:fd=None;f.write(_pack({'root':str(INFRA),'target':'amiberry-030','lease':local_lease}));f.flush();os.fsync(f.fileno())
 finally:
  if fd is not None:os.close(fd)
 save('preflight.json',result)
 env={**os.environ,'AMIGA_LAUNCH_LEASE':str(HERE/'lease.private.json')}
 result['startCalls']=1
 r=subprocess.run([binding()['python'], '-B', str(INFRA/'scripts/emulator.py'), 'start', '--target', 'amiberry-030'],cwd=INFRA,env=env,capture_output=True,timeout=110)
 (HERE/'start.stdout').write_bytes(r.stdout);(HERE/'start.stderr').write_bytes(r.stderr)
 result['returncode']=r.returncode;assert r.returncode==0,'First startup failure retained; no retry or cleanup'
 result['ownedProcess']=processes();assert len(result['ownedProcess'])==1
 result['status']='PASS_ONE_OWNED_030_START'
except BaseException as e:
 result.update(status='FIRST_FAILURE_RETAINED_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
 result['holdAttempt']=hold_start_failure(local_guard,local_lease,controller,reservation_binding)
finally:
 result['guardAfter']=LauncherGuard(INFRA).status();result['completedUTC']=datetime.datetime.now(datetime.timezone.utc).isoformat();save('start-terminal.json',result)
print(json.dumps({k:result[k] for k in ('status','startCalls','reserveCalls')}))
if result['status']!='PASS_ONE_OWNED_030_START':raise SystemExit(20)
