"""Private one-window caller. Importing performs no target exchange."""
import asyncio,contextlib,datetime,fcntl,hashlib,json,os,stat,subprocess,sys,urllib.request
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
REPO=ROOT/'work/Protracker-2.4A'
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
BASE=HERE.parent/'v2'
NATIVE=HERE.parent/'native-window-v1'
COMMIT='b13d6e70bcecb6813ad59ea43b0673df0280b942'
sys.path[:0]=[str(REPO/'tools'),str(INFRA/'scripts')]
import editor_fixture_physical_protocol as protocol
from launcher_guard import LauncherGuard,_pack
from launcher_access import operation,install_client,install_ipc,lease_input
from bridge_checks import target,require_reply
from emulator import processes,owns

def now():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def save(name,value):
 with (HERE/name).open('x') as f:json.dump(value,f,indent=2);f.write('\n')
def load(name):return json.loads((HERE/name).read_text())
def fp(path):
 p=Path(path);s=p.lstat();assert stat.S_ISREG(s.st_mode) and not s.st_flags&0x40000000,p
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def health():
 with urllib.request.urlopen('http://127.0.0.1:3000/health',timeout=3) as r:j=json.load(r)
 assert j['status']=='ok'
 return {k:j['serial'][k] for k in ('host','port','connected')}
def endpoint(name):
 import tomllib
 p=tomllib.loads((INFRA/'config/devbench.toml').read_text())['profiles'][name]
 return p['host'],p['port']
def absent_emulator():
 assert not processes(),'Any existing emulator refuses this phase'
 r=subprocess.run(['/usr/sbin/lsof','-nP','-iTCP:2345','-sTCP:LISTEN'],capture_output=True,timeout=5)
 assert r.returncode==1 and not r.stdout and not r.stderr
@contextlib.contextmanager
def locks():
 fds=[]
 try:
  for name in ('test.lock','real-a1200-safari.lock'):
   fd=os.open(INFRA/'runtime'/name,os.O_RDWR|os.O_NOFOLLOW);fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);fds.append(fd)
  # Probe only; the shared exchange/release API owns this third lock itself.
  fd=os.open(INFRA/'runtime/launcher-transport.lock',os.O_RDWR|os.O_NOFOLLOW)
  try:fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB)
  finally:os.close(fd)
  yield
 finally:
  for fd in reversed(fds):os.close(fd)
def custody():
 for p,h in load('pins.json')['files'].items():assert fp(p)==h,p
 controls=json.loads((BASE/'source-controls.json').read_text())['protected']
 for n,h in controls.items():
  p=REPO/n;assert fp(p)['sha256']==h['sha256'] and p.lstat().st_mode==h['mode'],n
 assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=REPO,text=True).strip()==COMMIT
 assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=REPO)
 build=json.loads((BASE/'native/manifest.json').read_text())
 assert build['source_before']==build['source_after'] and len(build['source_before'])==909
 for n,h in build['source_before'].items():assert fp(BASE/'candidate'/n)['sha256']==h,n
 for p,h in build['dependencies'].items():assert fp(p)=={k:h[k] for k in ('bytes','sha256')},p
 binary=Path(build['product']['path']);expected=BASE/'host/bridges-run.stdout'
 native=json.loads((NATIVE/'run-terminal.json').read_text());release=json.loads((NATIVE/'root-verified-release.json').read_text())
 protocol.qualify(native,release,binary.read_bytes(),expected.read_bytes())
 assert fp(REPO/'tools/editor_fixture_physical_protocol.py')==load('host-protocol-proof.json')['testedSources']['tools/editor_fixture_physical_protocol.py']
 return binary,expected,native,release
def peer_gate():
 c=load('coordination.json');assert c['status']=='FRESH_ROOT_CANARY_SCOPE_ALL_PEERS_HANDS_OFF'
 assert set(c['peers'])=={'AmiConnect','Scott','AmiGUS'}
 assert all(p['admissionConfirmed'] is True for p in c['peers'].values())
 assert load('preexecution-review.json')['status']=='PASS_ROOT_SELF_PREEXECUTION_REVIEW'
def safari_gate(name,max_age=180):
 j=load(name);assert j['status']=='AUTHENTICATED_VIEW_ONLY_VERIFIED' and not j['controlAcquired']
 assert j['url']=='http://192.168.0.156:8080/'
 t=datetime.datetime.fromisoformat(j['atUTC'])
 assert 0<=(datetime.datetime.now(datetime.timezone.utc)-t).total_seconds()<=max_age
 return j
def own(name):
 os.environ['AMIGA_LAUNCH_LEASE']=str(HERE/('physical.lease.private.json' if name=='real-a1200' else 'return.lease.private.json'))
 state=LauncherGuard(INFRA).status();lease=lease_input(INFRA,name)
 assert state['phase']=='READY' and not state['inflight'] and not state['holds']
 assert state['reservation']['lease']['run_id']==lease['run_id'] and state['reservation']['target']==name
 return state,lease
def reserve(name,purpose):
 guard=LauncherGuard(INFRA)
 fd=os.open(HERE/('physical.lease.private.json' if name=='real-a1200' else 'return.lease.private.json'),os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 try:
  lease=guard.reserve(name,'ProTracker root01a0b5ae-c04f-7661-8859-e89df20c3290',purpose,
                      {'sourceCommit':COMMIT,'candidateSHA256':protocol.SHA,'scopeSHA256':fp(HERE/'scope-plan.json')['sha256'],
                       'pinsSHA256':fp(HERE/'pins.json')['sha256'],'coordinationSHA256':fp(HERE/'coordination.json')['sha256']})
  with os.fdopen(fd,'wb') as f:fd=None;f.write(_pack({'root':str(INFRA),'target':name,'lease':lease}));f.flush();os.fsync(f.fileno())
 finally:
  if fd is not None:os.close(fd)
 return lease
def live(name):
 target(INFRA,name)
 # Every caller already has an admitted frame. The hooks validate its nonce
 # immediately before each actual exchange; saved custody is checked again here.
 custody()
 if name=='real-a1200':assert not processes()
async def call(session,name,args,seconds,audit,profile='real-a1200'):
 live(profile)
 item={'tool':name,'arguments':args,'status':'DISPATCH_ATTEMPTED'};audit.append(item)
 result=await asyncio.wait_for(session.call_tool(name,args),seconds)
 item.update(response='\n'.join(x.text for x in result.content if hasattr(x,'text')),isError=result.isError,status='REPLY_OBSERVED')
 return require_reply(result)
def no_candidate(text):
 assert text.startswith('[OK]\n')
 for token in ('PTEditorBridgesTest','AmiGUSTest','AmiGUSPlayer','AHITest','PTNativeMixed'):
  assert token.lower() not in text.lower(),'Active known candidate/player/test; stop'
def records_release(prefix,lease,target_name,before,records):
 assert set(records)=={'authorization','restoration','absence','cleanup'}
 for n,j in records.items():save(prefix+'-'+n+'.json',j)
 hashes={n:fp(HERE/(prefix+'-'+n+'.json'))['sha256'] for n in records}
 proof={'run_id':lease['run_id'],'target':target_name,'fence':before['fence'],
        'externally_verified':True,'records':hashes,'facts':{n:True for n in records}}
 save(prefix+'-proof.json',proof)
 guard=LauncherGuard(INFRA);assert guard.status()==before
 released=guard.release(lease,proof);after=guard.status()
 assert after['phase']=='EMPTY' and after['fence']==before['fence']+1 and not after['reservation'] and not after['holds'] and not after['inflight'] and not after['potential_resident']
 return released,after,hashes
