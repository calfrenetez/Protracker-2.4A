"""Independent physical absence/disconnect/release;030 return stays pending."""
from window_common import *
from mcp import ClientSession
from mcp.client.streamable_http import streamable_http_client
assert not (HERE/'physical-verified-release.json').exists()
async def main():
 r={'status':'PREPARING','calls':[],'atUTC':now(),'overallRestoration':'PENDING_ORIGINAL030_RETURN','fullWindowReleased':False}
 try:
  with locks():
   _,expected,_,_=custody();peer_gate();safari=safari_gate('safari-after.json');before,lease=own('real-a1200');live('real-a1200')
   run=load('physical-run.json');settled=load('physical-settlement.json')
   assert settled['status']=='PASS_COMPLETED_PHYSICAL_FIXTURE_FILES_ABSENT'
   directory=HERE/'physical-custody';assert {p.name for p in directory.iterdir()}==set(protocol.FILES)
   for n,h in run['files'].items():assert fp(directory/n)==h,n
   install_client()
   with operation(INFRA,'real-a1200','Independent completed physical fixture absence and disconnect',may_leave_running=True):
    async with streamable_http_client('http://127.0.0.1:3000/mcp') as (rd,wr,_):
     async with ClientSession(rd,wr) as s:
      await s.initialize()
      tasks=await call(s,'amiga_run_script',{'script':'Status FULL','timeout':10},20,r['calls']);no_candidate(tasks);r['independentTasks']=tasks
      absent=await call(s,'amiga_run_script',{'script':protocol.absence_script(run['namespace']),'timeout':10},20,r['calls'])
      assert protocol.reply(absent)==['PT-ABSENT'];r['independentAbsence']=absent
      disconnected=await call(s,'amiga_disconnect',{},20,r['calls']);assert disconnected=='Disconnected'
   guard=LauncherGuard(INFRA);before=guard.status();assert before['phase']=='READY' and not before['holds'] and not before['inflight']
   absent_emulator();custody()
   host,port=endpoint('real-a1200');assert health()=={'host':host,'port':port,'connected':False}
   records={'authorization':{'scope':fp(HERE/'scope-plan.json'),'coordination':fp(HERE/'coordination.json'),'humanPhysicalTestingAuthorized':True},
            'restoration':{'physicalInstalledConfigAndPlayersUnmodified':True,'DevBenchDisconnectedPhysicalEndpoint':True,'Safari':safari,
                           'original030TransportReturn':'PENDING_RETAINED_WINDOW','fullSharedRestoration':False},
            'absence':{'independentPhysicalDirectoryAbsent':True,'independentCandidateTasksAbsent':True,'allEmulatorsAbsent':True,'DevBenchDisconnected':True},
            'cleanup':{'completedOnlyExactSixFileCustody':run['files'],'settlement':fp(HERE/'physical-settlement.json'),'noFailedCleanupRetryRecoveryResetOrAudio':True}}
   released,after,hashes=records_release('physical-release',lease,'real-a1200',before,records)
   r.update(status='PASS_PHYSICAL_SCOPE_RELEASE_ORIGINAL030_RETURN_PENDING',release=released,guardAfter=after,records=hashes)
 except BaseException as e:r.update(status='FIRST_PHYSICAL_RELEASE_FAILURE_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500],guardAfter=LauncherGuard(INFRA).status())
 finally:r['completedUTC']=now();save('physical-verified-release.json',r)
 print(r['status'],flush=True)
 return 0 if r['status']=='PASS_PHYSICAL_SCOPE_RELEASE_ORIGINAL030_RETURN_PENDING' else 20
raise SystemExit(asyncio.run(main()))
