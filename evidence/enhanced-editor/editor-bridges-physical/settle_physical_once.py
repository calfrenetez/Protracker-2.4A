"""Separate completed-only exact RAM cleanup; no control/restoration/release."""
from window_common import *
from mcp import ClientSession
from mcp.client.streamable_http import streamable_http_client
assert not (HERE/'physical-settlement.json').exists()
async def main():
 r={'status':'PREPARING','calls':[],'atUTC':now()}
 try:
  with locks():
   _,expected,_,_=custody();peer_gate();own('real-a1200');live('real-a1200')
   run=load('physical-run.json');assert run['status']=='PASS_EXACT_PHYSICAL_SOFTWARE_FIXTURE_SETTLEMENT_PENDING' and run['launches']==1
   directory=HERE/'physical-custody';assert str(directory)==run['custodyDirectory'] and {p.name for p in directory.iterdir()}==set(protocol.FILES)
   data={n:(directory/n).read_bytes() for n in protocol.FILES}
   for n,h in run['files'].items():assert fp(directory/n)==h,n
   install_client()
   with operation(INFRA,'real-a1200','Separate completed-only editor fixture RAM settlement',may_leave_running=True):
    async with streamable_http_client('http://127.0.0.1:3000/mcp') as (rd,wr,_):
     async with ClientSession(rd,wr) as s:
      await s.initialize()
      async def cb(name,args,seconds):return await call(s,name,args,seconds,r['calls'])
      tasks=await cb('amiga_run_script',{'script':'Status FULL','timeout':10},20);no_candidate(tasks);r['tasksBefore']=tasks
      r.update(await protocol.settle_completed(run,data,expected.read_bytes(),cb))
      r['overallRestoration']='PENDING_PHYSICAL_INDEPENDENT_RELEASE_AND_ORIGINAL030_RETURN'
 except BaseException as e:r.update(status='FIRST_PHYSICAL_SETTLEMENT_FAILURE_HOLD_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
 finally:r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=now();save('physical-settlement.json',r)
 print(r['status'],flush=True)
 return 0 if r['status']=='PASS_COMPLETED_PHYSICAL_FIXTURE_FILES_ABSENT' else 20
raise SystemExit(asyncio.run(main()))
