"""One separately admitted030 startup for the new RAM/script transport canary."""
from window_common import *
assert not (HERE/'start-terminal.json').exists()
r={'status':'PREPARING','reserveCalls':0,'startCalls':0,'atUTC':now(),'physicalTouched':False,'candidateLaunched':False}
try:
 with locks():
  custody();peer_gate();absent_emulator()
  prior=load('prior-release.json');assert fp(prior['path'])==prior['fingerprint']
  before=LauncherGuard(INFRA).status();assert before==prior['guardAfter'] and before['phase']=='EMPTY' and not before['reservation'] and not before['inflight'] and not before['holds'] and not before['potential_resident']
  original=health();assert original=={'host':'127.0.0.1','port':2345,'connected':False}
  flag=INFRA/'runtime/Dev/Tools/bridge-enabled';assert flag.read_bytes()==b''
  r.update(guardBefore=before,originalEndpoint=original,originalBridgeFlag=fp(flag));r['reserveCalls']=1
  lease=reserve('amiberry-030','Fresh bounded RAM/script transport-only canary before physical protocol; inert marker file never executed, no candidate rerun')
  r['namespace']=protocol.namespace(lease['run_id'])
 os.environ['AMIGA_LAUNCH_LEASE']=str(HERE/'return.lease.private.json');r['startCalls']=1
 p=subprocess.run([sys.executable,'-B',str(INFRA/'scripts/emulator.py'),'start','--target','amiberry-030'],cwd=INFRA,env=os.environ.copy(),capture_output=True,timeout=110)
 (HERE/'start.stdout').open('xb').write(p.stdout);(HERE/'start.stderr').open('xb').write(p.stderr)
 r['returncode']=p.returncode;assert p.returncode==0,'First canary startup failure retained, no automatic closure/retry'
 r['ownedProcess']=processes();assert len(r['ownedProcess'])==1 and owns(r['ownedProcess'][0][1],'amiberry-030')
 r['status']='PASS_ONE_OWNED030_TRANSPORT_CANARY_START'
except BaseException as e:r.update(status='FIRST_CANARY_START_FAILURE_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
finally:r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=now();save('start-terminal.json',r)
print(r['status'])
if r['status']!='PASS_ONE_OWNED030_TRANSPORT_CANARY_START':raise SystemExit(20)
