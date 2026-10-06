"""Changed host-only independent release verifier; original self-lock refusal retained, no target replay."""
import datetime,fcntl,hashlib,json,os,stat,subprocess,sys,urllib.request
from pathlib import Path
HERE=Path(__file__).resolve().parent
from window_common import check_custody,check_complete,COMMIT
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
os.environ['AMIGA_LAUNCH_LEASE']=str(HERE/'lease.private.json');sys.path.insert(0,str(INFRA/'scripts'))
from launcher_guard import LauncherGuard
from launcher_access import lease_input
from emulator import processes

def fp(p):
 p=Path(p);s=p.lstat();assert stat.S_ISREG(s.st_mode) and not stat.S_ISLNK(s.st_mode) and not s.st_flags&0x40000000
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def save(name,value):
 with (HERE/name).open('x') as f:json.dump(value,f,indent=2);f.write('\n')
assert not (HERE/'root-verified-release.json').exists()
locks=[]
try:
 run=json.loads((HERE/'run-terminal.json').read_text());settled=json.loads((HERE/'settlement-terminal.json').read_text());started=json.loads((HERE/'start-terminal.json').read_text())
 assert run['status']=='PASS_EXACT_030_SOFTWARE_FIXTURE_SETTLEMENT_PENDING' and run['launches']==1 and run['completeAssertions']==check_complete((HERE/'fixture.log').read_bytes())
 assert settled['status']=='PASS_COMPLETED_FIXTURE_SETTLEMENT_RELEASE_PENDING' and not settled['deletions'] and not settled['forcedKills'] and not settled['failureTargetRequests']
 for name in ('test.lock','real-a1200-safari.lock'):
  fd=os.open(INFRA/'runtime'/name,os.O_RDWR|os.O_NOFOLLOW);fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);locks.append(fd)
 probe=os.open(INFRA/'runtime/launcher-transport.lock',os.O_RDWR|os.O_NOFOLLOW)
 try:fcntl.flock(probe,fcntl.LOCK_EX|fcntl.LOCK_NB)
 finally:os.close(probe)
 # The release API owns the transport lock itself; never hold a conflicting probe FD.
 guard=LauncherGuard(INFRA);before=guard.status();assert before==settled['guardAfter'] and before['phase']=='READY' and not before['inflight'] and not before['holds']
 lease=lease_input(INFRA,'amiberry-030');assert before['reservation']['lease']['run_id']==lease['run_id'] and before['reservation']['binding']['sourceCommit']==COMMIT
 assert not processes()
 p=subprocess.run(['/usr/sbin/lsof','-nP','-iTCP:2345','-sTCP:LISTEN'],capture_output=True,timeout=5);assert p.returncode==1 and not p.stdout
 with urllib.request.urlopen('http://127.0.0.1:3000/health',timeout=3) as response:health=json.load(response)
 assert health['status']=='ok' and health['serial']['connected'] is False and (health['serial']['host'],health['serial']['port'])==('127.0.0.1',2345)
 product,pinned_count=check_custody()
 import http.client
 conn=http.client.HTTPConnection('127.0.0.1',3000,timeout=3)
 try:
  conn.request('GET','/api/status');resp=conn.getresponse();denial=resp.read(65537)
  assert resp.status==403 and len(denial)<=65536 and guard.status()==before
 finally:conn.close()
 assert fp(INFRA/'runtime/Dev/Tools/bridge-enabled')==started['bridgeFlagBefore']
 custody=settled['custody'];assert not Path(custody['original']).exists()
 d=Path(custody['directory']);assert d==HERE/'stage-custody' and {p.name for p in d.iterdir()}==set(custody['files'])
 for name,pin in custody['files'].items():assert fp(d/name)==pin
 for name,pin in run['downloadedLogs'].items():assert fp(HERE/name)==pin
 save('release-v2-authorization.json',{'humanSource':'Direct instruction to keep working until a decision is needed, plus prior development/test/commit/push authorization','coordination':fp(HERE/'coordination.json'),'scope':fp(HERE/'scope-plan.json'),'candidate':run['candidate'],'sourceCommit':COMMIT,'originalFailurePreserved':True,'firstHostRefusal':fp(HERE/'first-release-refusal.json'),'changedHostVerifier':fp(HERE/'verify_release_v2.py'),'changedVerifierReview':fp(HERE/'release-v2-review.json')})
 save('release-v2-restoration.json',{'allPinnedInputsUnchanged':pinned_count,'DevBenchDisconnectedOriginalEndpoint':True,'bridgeFlagUnchanged':True,'noServiceRestartResetOrPhysicalAction':True,'ownedEmulatorClosedByNormalWrapper':True})
 save('release-v2-absence.json',{'allEmulatorsAbsent':True,'port2345ListenerAbsent':True,'DevBenchDisconnected':True,'candidateAbsentInPriorGuestStatusAndOwnedProcessGone':True,'physicalTouched':False})
 save('release-v2-cleanup.json',{'exactStageAbsent':True,'custodyBytesPreserved':custody,'noDeletionForceKillOrRetry':True,'completedOnlySettlement':fp(HERE/'settlement-terminal.json')})
 records={name:fp(HERE/('release-v2-'+name+'.json'))['sha256'] for name in ('authorization','restoration','absence','cleanup')}
 proof={'run_id':lease['run_id'],'target':'amiberry-030','fence':before['fence'],'externally_verified':True,'records':records,'facts':{name:True for name in records}}
 save('release-v2-proof.json',proof)
 assert fp(HERE/'verify_release_v2.py')==json.loads((HERE/'release-v2-review.json').read_text())['script']
 assert not processes() and guard.status()==before
 release=guard.release(lease,proof);after=guard.status();assert after['phase']=='EMPTY' and after['fence']==before['fence']+1 and not after['reservation'] and not after['inflight'] and not after['holds'] and not after['potential_resident']
 save('root-verified-release.json',{'status':'PASS_COMPLETED_DIAGNOSTIC_FIXTURE_VERIFIED_RELEASE','release':release,'guardAfter':after,'independentReadback':{'allEmulatorsAbsent':True,'listenerAbsent':True,'DevBenchDisconnected':True,'exactCustodyPreserved':True,'pinnedInputsUnchanged':pinned_count},'records':records,'nativeQualification':'Portable editor legacy15 + checked39/alias18 + bridges9/alias75 assertion bodies only; injected timers/voices/ordinary-RAM bus, no real clock/device/audio/timing/placement/totalstack/listening proof','firstHostReleaseRefusalPreserved':fp(HERE/'first-release-refusal.json'),'changedReleaseOnlyVerifier':True,'physical':'NOT_RUN','originalProgressFixtureStartup':'FAILED, unchanged; distinct old candidate never executed','atUTC':datetime.datetime.now(datetime.timezone.utc).isoformat()})
 print(json.dumps(release))
finally:
 for fd in reversed(locks):os.close(fd)
