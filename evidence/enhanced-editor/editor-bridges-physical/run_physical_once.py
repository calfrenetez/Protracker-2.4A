"""One physical connect/identity/exact fixture. First failure retains ownership."""
from window_common import *
from mcp import ClientSession
from mcp.client.streamable_http import streamable_http_client
assert not (HERE/'physical-run.json').exists()
async def main():
 r={'status':'PREPARING','connectCalls':0,'target':'real-a1200','launches':0,'calls':[],'atUTC':now()}
 try:
  with locks():
   binary,expected,native,release=custody();peer_gate();safari_gate('safari-before.json');absent_emulator()
   admission=load('physical-admission.json');assert admission['status']=='PASS_PHYSICAL_ONLY_ADMISSION_NOT_CONNECTED'
   own('real-a1200');assert health()==admission['originalEndpoint']
   install_client()
   with operation(INFRA,'real-a1200','One physical editor bridge RAM fixture, no device/audio/timing activation',may_leave_running=True):
    async with streamable_http_client('http://127.0.0.1:3000/mcp') as (rd,wr,_):
     async with ClientSession(rd,wr) as s:
      await s.initialize()
      assert health()==admission['originalEndpoint'];custody();absent_emulator()
      host,port=endpoint('real-a1200');r['connectCalls']=1
      result=await asyncio.wait_for(s.call_tool('amiga_connect',{'mode':'tcp','host':host,'port':port}),20)
      r['connectFirstReply']='\n'.join(x.text for x in result.content if hasattr(x,'text'));r['connectFirstIsError']=result.isError
      text=require_reply(result)
      assert text==f'Connected via TCP to {host}:{port}';r['connectResponse']=text;live('real-a1200')
      async def cb(name,args,seconds):
       if name=='amiga_run_script' and args.get('script','').startswith('Run '):r['launches']+=1
       return await call(s,name,args,seconds,r['calls'])
      ping=await cb('amiga_ping',{},20);assert ping.startswith('Amiga alive');r['pingBefore']=ping
      identity=await cb('amiga_run_script',{'script':'Version\nCPU\nAvail\nVersion bsdsocket.library\nShowNetStatus\n','timeout':20},30)
      assert identity.startswith('[OK]\n') and '68030' in identity and "192.168.0.156 (on interface 'plipbox')" in identity
      r['identity']=identity
      tasks=await cb('amiga_run_script',{'script':'Status FULL','timeout':10},20);no_candidate(tasks);r['tasksBefore']=tasks
      q=json.loads((INFRA/'config/hardware.json').read_text());bridge=INFRA/'runtime/physical-staging/bridge-20260923'
      assert fp(bridge)=={'bytes':q['bridge_size'],'sha256':q['bridge_sha256']}
      protocol.checksum(await cb('amiga_checksum',{'path':q['bridge_path']},20),bridge.read_bytes());r['bridge']=fp(bridge)
      out=HERE/'physical-custody';out.mkdir()
      result=await protocol.execute_once(native,release,binary,expected,admission['namespace'],out,cb)
      r.update(result);r['custodyDirectory']=str(out);r['overallRestoration']='PENDING_ORIGINAL030_RETURN'
 except BaseException as e:r.update(status='FIRST_PHYSICAL_FAILURE_HOLD_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
 finally:r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=now();save('physical-run.json',r)
 print(r['status'],flush=True)
 return 0 if r['status']=='PASS_EXACT_PHYSICAL_SOFTWARE_FIXTURE_SETTLEMENT_PENDING' else 20
raise SystemExit(asyncio.run(main()))
