"""Fresh physical-only admission. No target probe or automatic continuation."""
from window_common import *
assert not (HERE/'physical-admission.json').exists()
r={'status':'PREPARING','reserveCalls':0,'targetCalls':0,'atUTC':now()}
try:
 with locks():
  custody();peer_gate();safari_gate('safari-before.json');absent_emulator()
  prior=load('prior-release.json');assert fp(prior['path'])==prior['fingerprint']
  guard=LauncherGuard(INFRA);before=guard.status()
  assert before==prior['guardAfter'] and before['phase']=='EMPTY' and not before['reservation'] and not before['inflight'] and not before['holds'] and not before['potential_resident']
  original=health();assert original=={'host':'127.0.0.1','port':2345,'connected':False}
  flag=INFRA/'runtime/Dev/Tools/bridge-enabled';assert flag.read_bytes()==b''
  r.update(guardBefore=before,originalEndpoint=original,originalBridgeFlag=fp(flag))
  r['reserveCalls']=1
  lease=reserve('real-a1200','Human-authorized exact RAM-only editor bridge fixture, physical-scope release followed by separate original030 return; full window retained')
  r.update(status='PASS_PHYSICAL_ONLY_ADMISSION_NOT_CONNECTED',namespace=protocol.namespace(lease['run_id']),overallRestoration='PENDING_ORIGINAL030_RETURN')
except BaseException as e:r.update(status='FIRST_ADMISSION_FAILURE_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
finally:r['guardAfter']=LauncherGuard(INFRA).status();save('physical-admission.json',r)
print(r['status'])
if not r['status'].startswith('PASS_'):raise SystemExit(20)
