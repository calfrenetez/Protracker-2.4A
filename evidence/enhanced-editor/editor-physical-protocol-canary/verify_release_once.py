"""Independent host readback and normal release. No target requests/replay."""
from window_common import *
assert not (HERE/'root-verified-release.json').exists()
with locks():
 custody();peer_gate();before,lease=own('amiberry-030');absent_emulator()
 run=load('run-terminal.json');settled=load('settlement-terminal.json');started=load('start-terminal.json')
 assert run['status']=='PASS_030_RAM_SCRIPT_CANARY_SETTLEMENT_PENDING' and run['shellLaunchAttempts']==1 and run['candidateLaunches']==0 and not run['physicalTouched']
 assert settled['status']=='PASS_COMPLETED_CANARY_SETTLEMENT_NORMAL_STOP_RELEASE_PENDING' and settled['normalStop'] and not settled['forcedKills'] and settled['candidateLaunches']==0 and not settled['physicalTouched']
 assert health()==started['originalEndpoint'] and fp(INFRA/'runtime/Dev/Tools/bridge-enabled')==started['originalBridgeFlag']
 directory=HERE/'canary-custody';assert {p.name for p in directory.iterdir()}==set(protocol.FILES)
 for n,h in run['files'].items():assert fp(directory/n)==h,n
 assert fp(HERE/'original030-idle.png')['bytes']>0
 records={'authorization':{'humanStandingTestingAuthorization':True,'scope':fp(HERE/'scope-plan.json'),'coordination':fp(HERE/'coordination.json')},
          'restoration':{'original030ProfileAndDisconnectedEndpointVerified':True,'bridgeFlagUnchanged':True,'originalIdleScreenshot':fp(HERE/'original030-idle.png'),'physicalAndBrowserUntouched':True},
          'absence':{'allEmulatorsAbsent':True,'listener2345Absent':True,'DevBenchDisconnected':True,'independentGuestAbsence':settled['independentAbsence']},
          'cleanup':{'exactSixCompletedCanaryFilesAbsent':True,'allCustodyBytesPreserved':run['files'],'completedOnlySettlement':fp(HERE/'settlement-terminal.json'),'noFailedCleanupRecoveryResetOrRetry':True}}
 released,after,hashes=records_release('canary-release',lease,'amiberry-030',before,records)
 save('root-verified-release.json',{'status':'PASS_030_RAM_SCRIPT_PROTOCOL_CANARY_VERIFIED_RELEASE','release':released,'guardAfter':after,'records':hashes,'candidateLaunches':0,'physicalTouched':False,
                                 'scope':'Literal script namespace/WAIT/COMPLETE/CRC transfer/cleanup on030 only; inert marker never executed, no new application/device/audio/timing/physical acceptance','atUTC':now()})
 print('PASS native RAM/script canary verified normal release, no ProTracker binary or physical operation')
