"""Separate original030 idle qualification, disconnect and bounded owned stop."""
from window_common import *
from mcp import ClientSession
from mcp.client.streamable_http import streamable_http_client
from amiberry_mcp.ipc_client import AmiberryIPCClient
from shared_guest import Guest
from emulator import main as emulator_main
assert not (HERE/'return-idle.json').exists()
async def main():
 r={'status':'PREPARING','calls':[],'atUTC':now(),'candidateLaunches':0,'overallRestoration':'PENDING_INDEPENDENT_FINAL_RELEASE'}
 try:
  with locks():
   custody();peer_gate();own('amiberry-030');started=load('return-start.json')
   assert started['status']=='PASS_ONE_ORIGINAL030_RETURN_START_IDLE_CHECK_PENDING'
   owned=[tuple(x) for x in started['ownedProcess']];assert processes()==owned
   install_client();install_ipc(INFRA)
   with operation(INFRA,'amiberry-030','Separate original030 idle return verification and normal owned shutdown',may_leave_running=True):
    c=AmiberryIPCClient(socket_path='/tmp/amiberry.sock');guest=Guest(INFRA,HERE)
    def same():
     assert processes()==owned;live('amiberry-030')
    same();r['cpu']=await c.get_cpu_model();same();r['memory']=await c.get_memory_config();same();r['chipset']=await c.get_chipset()
    assert r['cpu']['model']=='68030' and str(r['cpu']['fpu'])=='0'
    assert r['memory']['chip']=='2048KB' and r['memory']['z3']=='131072KB' and r['memory']['rtg']=='0KB' and r['chipset'][1].upper()=='AGA'
    same();r['dma']=await c.get_dma_state();assert all(r['dma']['audio'+str(i)]=='0' for i in range(4))
    same();r['statusBefore']=guest.command('GET_STATUS');assert 'Paused=false' in r['statusBefore']
    same();assert await asyncio.wait_for(c.screenshot(str(HERE/'original030-idle.png')),6)
    async with streamable_http_client('http://127.0.0.1:3000/mcp') as (rd,wr,_):
     async with ClientSession(rd,wr) as s:
      await s.initialize()
      same();ping=await call(s,'amiga_ping',{},20,r['calls'],'amiberry-030');assert ping.startswith('Amiga alive');r['ping']=ping
      same();tasks=await call(s,'amiga_run_script',{'script':'Status FULL','timeout':10},20,r['calls'],'amiberry-030');no_candidate(tasks);r['tasks']=tasks
      same();text=await call(s,'amiga_disconnect',{},20,r['calls'],'amiberry-030');assert text=='Disconnected'
    assert health()=={'host':'127.0.0.1','port':2345,'connected':False} and processes()==owned
    await emulator_main(['stop','--target','amiberry-030'])
    absent_emulator();custody();r['ownedProcess']=owned;r['normalStop']=True;r['forcedKills']=0
    r['status']='PASS_ORIGINAL030_IDLE_RETURN_AND_NORMAL_STOP_RELEASE_PENDING'
 except BaseException as e:r.update(status='FIRST_RETURN_IDLE_FAILURE_HOLD_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
 finally:r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=now();save('return-idle.json',r)
 print(r['status'],flush=True)
 return 0 if r['status']=='PASS_ORIGINAL030_IDLE_RETURN_AND_NORMAL_STOP_RELEASE_PENDING' else 20
raise SystemExit(asyncio.run(main()))
