"""Separate independent recovery readback, then normal exact HOLD release."""
from window_common import *
assert not (HERE/'recovery-root-verified-release.json').exists()
r={'status':'PREPARING','releaseCalls':0,'signals':0,'guestRequests':0,'physicalTouched':False,'atUTC':now()}
try:
 with locks():
  custody();peer_gate();closed=load('recovery-terminal.json');identity=load('host-only-failure-identity-v1.json');approval=load('explicit-recovery-approval.json')
  assert approval['approved'] is True and approval['pid']==93129 and approval['fence']==85 and approval['plan']==fp(HERE/'recovery-plan.json')
  assert closed['status']=='PASS_EXACT_CANARY_RECOVERY_CLOSURE_HOLD_RETAINED' and closed['sigtermCalls']==1 and not closed['forcedKills'] and not closed['guestRequests'] and not closed['deletions'] and not closed['releaseCalls']
  for p,h in load('recovery-pins.json')['files'].items():assert fp(p)==h,p
  guard=LauncherGuard(INFRA);before=guard.status();assert before==closed['guardBefore']==closed['guardAfter']==identity['guard'] and before['phase']=='HOLD' and before['fence']==85 and before['inflight'] is None
  os.environ['AMIGA_LAUNCH_LEASE']=str(HERE/'return.lease.private.json');lease=lease_input(INFRA,'amiberry-030');assert before['reservation']['lease']['run_id']==lease['run_id']
  absent_emulator();assert health()==identity['originalEndpoint'] and fp(INFRA/'runtime/Dev/Tools/bridge-enabled')==identity['bridgeFlag']
  run=load('run-terminal.json');settled=load('settlement-terminal.json');directory=HERE/'canary-custody';assert {p.name for p in directory.iterdir()}==set(protocol.FILES)
  for n,h in run['files'].items():assert fp(directory/n)==h,n
  records={'authorization':{'humanRecoveryApproval':fp(HERE/'explicit-recovery-approval.json'),'freshRecoveryCoordination':fp(HERE/'recovery-coordination.json'),'reviewedPlan':fp(HERE/'recovery-plan.json')},'restoration':{'originalHealthyDisconnectedEndpoint':identity['originalEndpoint'],'bridgeFlagUnchanged':identity['bridgeFlag'],'savedOriginalIdleSnapshot':fp(HERE/'original030-idle.png'),'physicalBrowserUntouched':True},'absence':{'allEmulatorsAbsent':True,'listener2345Absent':True,'DevBenchDisconnected':True,'savedIndependentGuestAbsence':settled['independentAbsence'],'candidateNeverExecuted':True},'cleanup':{'oneExactOwnSIGTERM':93129,'forcedKills':0,'guestRequests':0,'deletions':0,'savedCompletedOnlyExactSixFileCleanup':settled['cleanup'],'allFirstRecordsPreserved':load('recovery-pins.json')['files'],'closure':fp(HERE/'recovery-terminal.json'),'canaryReplay':False}}
  r['releaseCalls']=1;released,after,hashes=records_release('recovery-release',lease,'amiberry-030',before,records)
  r.update(status='PASS_EXACT_CANARY_RECOVERY_VERIFIED_RELEASE',release=released,guardAfter=after,records=hashes,originalFailurePreserved=True,canaryRuntime='PASS transport checks; first settlement host closure FAILED; distinct approved recovery completed',applicationPhysical='NOT_RUN')
except BaseException as e:r.update(status='FIRST_RECOVERY_RELEASE_FAILURE_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
finally:r['currentGuardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=now();save('recovery-root-verified-release.json',r)
print(r['status'])
if r['status']!='PASS_EXACT_CANARY_RECOVERY_VERIFIED_RELEASE':raise SystemExit(20)
