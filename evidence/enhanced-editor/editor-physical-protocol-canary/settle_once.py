"""Completed-only canary CRC/cleanup/independent absence/idle/disconnect/stop."""
from window_common import *
from mcp import ClientSession
from mcp.client.streamable_http import streamable_http_client
from amiberry_mcp.ipc_client import AmiberryIPCClient
from shared_guest import Guest
from emulator import main as emulator_main
from types import SimpleNamespace
assert not (HERE/'settlement-terminal.json').exists()
async def main():
 r={'status':'PREPARING','calls':[],'candidateLaunches':0,'physicalTouched':False,'atUTC':now()}
 try:
  with locks():
   custody();peer_gate();own('amiberry-030');run=load('run-terminal.json');started=load('start-terminal.json')
   assert run['status']=='PASS_030_RAM_SCRIPT_CANARY_SETTLEMENT_PENDING' and run['shellLaunchAttempts']==1 and run['candidateLaunches']==0
   owned=[tuple(x) for x in started['ownedProcess']];assert processes()==owned
   directory=HERE/'canary-custody';assert {p.name for p in directory.iterdir()}==set(protocol.FILES)
   for n,h in run['files'].items():assert fp(directory/n)==h,n
   install_client();install_ipc(INFRA)
   with operation(INFRA,'amiberry-030','Separate completed-only RAM/script canary settlement and normal owned closure',may_leave_running=True):
    c=AmiberryIPCClient(socket_path='/tmp/amiberry.sock');guest=Guest(INFRA,HERE)
    def same():assert processes()==owned;live('amiberry-030')
    async with streamable_http_client('http://127.0.0.1:3000/mcp') as (rd,wr,_):
     async with ClientSession(rd,wr) as s:
      await s.initialize()
      async def cb(name,args):same();return await call(s,name,args,20,r['calls'],'amiberry-030')
      tasks=await cb('amiga_run_script',{'script':'Status FULL','timeout':10});no_candidate(tasks);r['tasksBefore']=tasks
      for n in protocol.FILES:protocol.checksum(await cb('amiga_checksum',{'path':run['namespace']+'/'+n}),(directory/n).read_bytes())
      text=await cb('amiga_run_script',{'script':protocol.cleanup_script(run['namespace']),'timeout':10});assert protocol.reply(text)==['PT-CLEAN'];r['cleanup']=text
      text=await cb('amiga_run_script',{'script':protocol.absence_script(run['namespace']),'timeout':10});assert protocol.reply(text)==['PT-ABSENT'];r['independentAbsence']=text
      text=await cb('amiga_run_script',{'script':'Status FULL','timeout':10});no_candidate(text);r['independentTasks']=text
      same();r['dmaAfter']=await c.get_dma_state();assert all(r['dmaAfter']['audio'+str(i)]=='0' for i in range(4))
      same();r['statusAfter']=guest.command('GET_STATUS');assert 'Paused=false' in r['statusAfter']
      same();assert await asyncio.wait_for(c.screenshot(str(HERE/'original030-idle.png')),6)
      same();assert await cb('amiga_disconnect',{})=='Disconnected'
    assert health()==started['originalEndpoint'] and processes()==owned
    await emulator_main(SimpleNamespace(action='stop',target='amiberry-030',no_bridge=False))
    absent_emulator();custody();r.update(status='PASS_COMPLETED_CANARY_SETTLEMENT_NORMAL_STOP_RELEASE_PENDING',forcedKills=0,normalStop=True)
 except BaseException as e:r.update(status='FIRST_CANARY_SETTLEMENT_FAILURE_HOLD_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
 finally:r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=now();save('settlement-terminal.json',r)
 print(r['status'],flush=True)
 return 0 if r['status']=='PASS_COMPLETED_CANARY_SETTLEMENT_NORMAL_STOP_RELEASE_PENDING' else 20
raise SystemExit(asyncio.run(main()))
