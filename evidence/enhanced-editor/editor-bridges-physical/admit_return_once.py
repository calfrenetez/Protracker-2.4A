"""New030 return-only lease/start AFTER verified physical-scope release."""
from window_common import *
assert not (HERE/'return-start.json').exists()
r={'status':'PREPARING','reserveCalls':0,'startCalls':0,'target':'amiberry-030','atUTC':now(),'overallRestoration':'PENDING_ORIGINAL030_RETURN'}
try:
 with locks():
  custody();peer_gate();safari_gate('safari-after.json');absent_emulator()
  physical=load('physical-verified-release.json');assert physical['status']=='PASS_PHYSICAL_SCOPE_RELEASE_ORIGINAL030_RETURN_PENDING' and not physical['fullWindowReleased']
  before=LauncherGuard(INFRA).status();assert before==physical['guardAfter'] and before['phase']=='EMPTY'
  host,port=endpoint('real-a1200');assert health()=={'host':host,'port':port,'connected':False}
  flag=INFRA/'runtime/Dev/Tools/bridge-enabled';assert fp(flag)==load('physical-admission.json')['originalBridgeFlag'] and flag.read_bytes()==b''
  r['guardBefore']=before;r['reserveCalls']=1
  reserve('amiberry-030','Separate return-only original030 profile/endpoint/idle verification after completed physical RAM fixture; no candidate replay')
 os.environ['AMIGA_LAUNCH_LEASE']=str(HERE/'return.lease.private.json')
 r['startCalls']=1
 p=subprocess.run([sys.executable,'-B',str(INFRA/'scripts/emulator.py'),'start','--target','amiberry-030'],cwd=INFRA,env=os.environ.copy(),capture_output=True,timeout=110)
 (HERE/'return-start.stdout').open('xb').write(p.stdout);(HERE/'return-start.stderr').open('xb').write(p.stderr)
 r['returncode']=p.returncode;assert p.returncode==0,'First return startup failure retained; no retry/stop/reconnect'
 r['ownedProcess']=processes();assert len(r['ownedProcess'])==1 and owns(r['ownedProcess'][0][1],'amiberry-030')
 r['status']='PASS_ONE_ORIGINAL030_RETURN_START_IDLE_CHECK_PENDING'
except BaseException as e:r.update(status='FIRST_RETURN_START_FAILURE_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
finally:r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=now();save('return-start.json',r)
print(r['status'])
if r['status']!='PASS_ONE_ORIGINAL030_RETURN_START_IDLE_CHECK_PENDING':raise SystemExit(20)
