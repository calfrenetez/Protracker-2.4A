"""Independent host-only final release AFTER completed original030 restoration."""
from window_common import *
assert not (HERE/'root-verified-release.json').exists()
with locks():
 custody();peer_gate();before,lease=own('amiberry-030');absent_emulator()
 idle=load('return-idle.json');assert idle['status']=='PASS_ORIGINAL030_IDLE_RETURN_AND_NORMAL_STOP_RELEASE_PENDING' and idle['normalStop'] and not idle['forcedKills'] and idle['candidateLaunches']==0
 physical=load('physical-verified-release.json');assert physical['status']=='PASS_PHYSICAL_SCOPE_RELEASE_ORIGINAL030_RETURN_PENDING'
 run=load('physical-run.json');settled=load('physical-settlement.json');admitted=load('physical-admission.json')
 assert run['status']=='PASS_EXACT_PHYSICAL_SOFTWARE_FIXTURE_SETTLEMENT_PENDING' and run['launches']==1 and settled['status']=='PASS_COMPLETED_PHYSICAL_FIXTURE_FILES_ABSENT'
 assert health()==admitted['originalEndpoint'] and fp(INFRA/'runtime/Dev/Tools/bridge-enabled')==admitted['originalBridgeFlag']
 directory=HERE/'physical-custody';assert {p.name for p in directory.iterdir()}==set(protocol.FILES)
 for n,h in run['files'].items():assert fp(directory/n)==h,n
 assert fp(HERE/'original030-idle.png')['bytes']>0
 records={'authorization':{'humanStandingDevelopmentTestingAuthorization':True,'scope':fp(HERE/'scope-plan.json'),'coordination':fp(HERE/'coordination.json')},
          'restoration':{'original030ProfileAndEndpointVerified':True,'originalDisconnectedEndpoint':health(),'physicalViewOnly':load('safari-after.json'),
                         'physicalInstalledConfigAndPreparedOtherMediaUnmodified':True,'bridgeFlagUnchanged':True,'original030ReturnObligation':'COMPLETE','noPhysicalAudioOrDeviceWrites':True},
          'absence':{'allEmulatorsAbsent':True,'port2345ListenerAbsent':True,'DevBenchDisconnected':True,'independentPhysicalRelease':fp(HERE/'physical-verified-release.json')},
          'cleanup':{'exactPhysicalSixFilesAbsentAndCustodyPreserved':run['files'],'completedOnlySettlement':fp(HERE/'physical-settlement.json'),'idleReturn':fp(HERE/'return-idle.json'),'noForcedKillFailedCleanupOrRetry':True}}
 released,after,hashes=records_release('final-release',lease,'amiberry-030',before,records)
 save('root-verified-release.json',{'status':'PASS_EXACT_PHYSICAL_SOFTWARE_FIXTURE_FULL_VERIFIED_RELEASE','release':released,'guardAfter':after,'records':hashes,
                                 'fullWindowReleased':True,'overallRestoration':'COMPLETE_ORIGINAL030_RETURN_VERIFIED','physicalScopeRelease':fp(HERE/'physical-verified-release.json'),
                                 'scope':'Physical CPU/software assertions with injected timers/voices/ordinary-RAM fakebus only; no placement/device/audio/timing/listening or full-native-UI acceptance','atUTC':now()})
 print('PASS full physical software fixture/window release with original030 restored')
