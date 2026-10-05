"""One fresh owned030 lifecycle; no retries, resets, service changes or recovery."""
import datetime,fcntl,hashlib,json,os,stat,subprocess,sys,urllib.request
from pathlib import Path
HERE=Path(__file__).resolve().parent;BASE=HERE.parent
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
try:
 pins=json.loads((HERE/'pins.json').read_text())['files']
 for p,h in pins.items():assert fp(p)==h,p
 build=json.loads((BASE/'native-v1/manifest.json').read_text());host=json.loads((BASE/'host-v1/execution.json').read_text())
 assert build['status']=='PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED' and host['status']=='PASS_HOST_ONLY' and build['source_stable'] and host['source_stable']
 product=build['product'];assert fp(product['path'])=={k:product[k] for k in ('bytes','sha256')}
 assert product['bytes']==292912 and product['sha256']=='2c88205c370a3ab8d68a0f5beecc851d0d9e44e9e90a95d590a74185d03348e8'
 assert json.loads((HERE/'coordination.json').read_text())['revision']>=20
 guard=LauncherGuard(INFRA);state=guard.status()
 assert state['phase']=='EMPTY' and not state['reservation'] and not state['inflight'] and not state['holds'] and not state['potential_resident']
 assert not processes(),'Any existing emulator refuses admission'
 with urllib.request.urlopen('http://127.0.0.1:3000/health',timeout=3) as r:health=json.load(r)
 assert health['status']=='ok' and health['serial']['connected'] is False and (health['serial']['host'],health['serial']['port'])==('127.0.0.1',2345)
 fds=[]
 try:
  for name in ('test.lock','real-a1200-safari.lock'):
   fd=os.open(INFRA/'runtime'/name,os.O_RDWR|os.O_NOFOLLOW);fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);fds.append(fd)
  assert guard.status()==state and not processes()
 finally:
  for fd in reversed(fds):os.close(fd)
 flag=INFRA/'runtime/Dev/Tools/bridge-enabled';assert flag.is_file() and flag.read_bytes()==b''
 result.update(guardBefore=state,healthBefore=health,bridgeFlagBefore=fp(flag),candidate=product,pinnedFiles=len(pins))
 fd=os.open(HERE/'lease.private.json',os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 try:
  result['reserveCalls']=1
  lease=guard.reserve('amiberry-030','ProTracker root01a0b5ae-c04f-7661-8859-e89df20c3290','Human authorized exact master-establishment software qualification; changed progress candidate coordinated scope',{'candidateSHA256':product['sha256'],'candidateBytes':product['bytes'],'sourceCommit':'120e51d6b90303484f6108a1c6b8ff5f2e6d10cb','evidence':str(HERE)})
  with os.fdopen(fd,'wb') as f:fd=None;f.write(_pack({'root':str(INFRA),'target':'amiberry-030','lease':lease}));f.flush();os.fsync(f.fileno())
 finally:
  if fd is not None:os.close(fd)
 save('preflight.json',result)
 env={**os.environ,'AMIGA_LAUNCH_LEASE':str(HERE/'lease.private.json')}
 result['startCalls']=1
 r=subprocess.run([sys.executable,'-B',str(INFRA/'scripts/emulator.py'),'start','--target','amiberry-030'],cwd=INFRA,env=env,capture_output=True,timeout=110)
 (HERE/'start.stdout').write_bytes(r.stdout);(HERE/'start.stderr').write_bytes(r.stderr)
 result['returncode']=r.returncode;assert r.returncode==0,'First startup failure retained; no retry or cleanup'
 result['ownedProcess']=processes();assert len(result['ownedProcess'])==1
 result['status']='PASS_ONE_OWNED_030_START'
except BaseException as e:
 result.update(status='FIRST_FAILURE_RETAINED_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
finally:
 result['guardAfter']=LauncherGuard(INFRA).status();result['completedUTC']=datetime.datetime.now(datetime.timezone.utc).isoformat();save('start-terminal.json',result)
print(json.dumps({k:result[k] for k in ('status','startCalls','reserveCalls')}))
if result['status']!='PASS_ONE_OWNED_030_START':raise SystemExit(20)
