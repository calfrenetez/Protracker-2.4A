"""One inert Shell canary; the ProTracker test executable is NEVER run here."""
from window_common import *
from mcp import ClientSession
from mcp.client.streamable_http import streamable_http_client
from amiberry_mcp.ipc_client import AmiberryIPCClient
import time
assert not (HERE/'run-terminal.json').exists()
async def main():
 r={'status':'PREPARING','calls':[],'shellLaunchAttempts':0,'candidateLaunches':0,'physicalTouched':False,'atUTC':now()}
 try:
  with locks():
   custody();peer_gate();own('amiberry-030');started=load('start-terminal.json');assert started['status']=='PASS_ONE_OWNED030_TRANSPORT_CANARY_START'
   owned=[tuple(x) for x in started['ownedProcess']];assert processes()==owned
   install_client();install_ipc(INFRA)
   with operation(INFRA,'amiberry-030','One RAM/script transport-only Shell canary; no ProTracker executable/audio',may_leave_running=True):
    c=AmiberryIPCClient(socket_path='/tmp/amiberry.sock')
    def same():
     assert processes()==owned;live('amiberry-030')
    same();r['cpu']=await c.get_cpu_model();same();r['memory']=await c.get_memory_config();same();r['chipset']=await c.get_chipset();same();r['dma']=await c.get_dma_state()
    assert r['cpu']['model']=='68030' and str(r['cpu']['fpu'])=='0' and r['memory']['chip']=='2048KB' and r['memory']['z3']=='131072KB' and r['memory']['rtg']=='0KB' and r['chipset'][1].upper()=='AGA'
    assert all(r['dma']['audio'+str(i)]=='0' for i in range(4))
    dest=started['namespace'];out=HERE/'canary-custody';out.mkdir()
    marker=b'INERT CANARY DATA; NEVER EXECUTED\n'
    script=('FailAt 21\nStack 65536\nCD '+dest+'\nEcho PT-RAM-PROTOCOL-CANARY >fixture.log\nEcho 0 >fixture.rc\nEcho COMPLETE >complete.flag\nQuit 0\n').encode('ascii')
    for name,b in ((protocol.BINARY,marker),('Run-once',script)):(out/name).open('xb').write(b)
    async with streamable_http_client('http://127.0.0.1:3000/mcp') as (rd,wr,_):
     async with ClientSession(rd,wr) as s:
      await s.initialize()
      async def cb(name,args,seconds=20):
       same();return await call(s,name,args,seconds,r['calls'],'amiberry-030')
      assert protocol.reply(await cb('amiga_run_script',{'script':protocol.create_script(dest),'timeout':10}))==['PT-CREATED']
      for name,b in ((protocol.BINARY,marker),('Run-once',script)):
       await cb('amiga_push_file',{'local_path':str(out/name),'amiga_path':dest+'/'+name},30)
       protocol.checksum(await cb('amiga_checksum',{'path':dest+'/'+name}),b)
      before=await cb('amiga_run_script',{'script':protocol.poll_script(dest),'timeout':10})
      assert protocol.reply(before)==['PT-WAIT'];r['nativeWAIT']=before
      start=time.monotonic();deadline=start+60;r['shellLaunchAttempts']=1
      launch=await cb('amiga_run_script',{'script':'Run >'+dest+'/launcher.log <NIL: Execute '+dest+'/Run-once','timeout':10})
      protocol.launch_reply(launch);r['launchReply']=launch
      while True:
       remaining=deadline-time.monotonic();assert remaining>0,'Absolute60second Shell canary deadline'
       text=await cb('amiga_run_script',{'script':protocol.poll_script(dest),'timeout':min(10,remaining)},min(20,remaining))
       assert time.monotonic()<=deadline,'Completion arrived after canary deadline'
       if protocol.complete(text):r['nativeCOMPLETE']=text;break
       await asyncio.sleep(min(1,max(0,deadline-time.monotonic())))
      r['elapsedSeconds']=time.monotonic()-start
      for n in ('fixture.log','fixture.rc','complete.flag','launcher.log'):
       p=out/n;await cb('amiga_pull_file',{'amiga_path':dest+'/'+n,'local_path':str(p)},30)
       assert p.is_file() and not p.is_symlink() and p.stat().st_size<=4096
       protocol.checksum(await cb('amiga_checksum',{'path':dest+'/'+n}),p.read_bytes())
      assert (out/'fixture.log').read_bytes()==b'PT-RAM-PROTOCOL-CANARY\n' and (out/'fixture.rc').read_bytes()==b'0\n' and (out/'complete.flag').read_bytes()==b'COMPLETE\n'
      assert 0<(out/'launcher.log').stat().st_size<=4096 and {p.name for p in out.iterdir()}==set(protocol.FILES)
      r.update(status='PASS_030_RAM_SCRIPT_CANARY_SETTLEMENT_PENDING',namespace=dest,files={n:fp(out/n) for n in protocol.FILES},custodyDirectory=str(out))
 except BaseException as e:r.update(status='FIRST_CANARY_FAILURE_HOLD_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
 finally:r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=now();save('run-terminal.json',r)
 print(r['status'],flush=True)
 return 0 if r['status']=='PASS_030_RAM_SCRIPT_CANARY_SETTLEMENT_PENDING' else 20
raise SystemExit(asyncio.run(main()))
