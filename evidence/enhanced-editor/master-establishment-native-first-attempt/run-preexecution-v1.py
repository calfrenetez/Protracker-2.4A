"""One exact portable software fixture; keep first failure resident, no automatic cleanup."""
import asyncio,datetime,fcntl,hashlib,json,os,stat,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;BASE=HERE.parent
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
STAGE=INFRA/'runtime/Dev/Tests/PT-master-establish-20261005-v1'
os.environ['AMIGA_LAUNCH_LEASE']=str(HERE/'lease.private.json')
sys.path.insert(0,str(INFRA/'scripts'))
from launcher_access import operation,transport,install_client,install_ipc
from launcher_guard import LauncherGuard
from emulator import processes,owns
from shared_guest import Guest
from bridge_checks import target,require_reply,checksum
from amiberry_mcp.ipc_client import AmiberryIPCClient
from mcp import ClientSession
from mcp.client.streamable_http import streamable_http_client

def fp(p):
 p=Path(p);s=p.lstat();assert stat.S_ISREG(s.st_mode) and not s.st_flags&0x40000000
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def save(name,value):
 with (HERE/name).open('x') as f:json.dump(value,f,indent=2);f.write('\n')
async def main():
 assert not (HERE/'run-terminal.json').exists()
 r={'status':'IN_PROGRESS','target':'amiberry-030','launches':0,'stage':str(STAGE),'atUTC':datetime.datetime.now(datetime.timezone.utc).isoformat(),'scope':'ordinary-memory software fixture with fake backend callbacks; no audio/device/nativeIRQ activation'}
 try:
  started=json.loads((HERE/'start-terminal.json').read_text());assert started['status']=='PASS_ONE_OWNED_030_START'
  pins=json.loads((HERE/'pins.json').read_text())['files']
  for p,h in pins.items():assert fp(p)==h,p
  product=json.loads((BASE/'native-v1/manifest.json').read_text())['product'];assert fp(product['path'])=={k:product[k] for k in ('bytes','sha256')}
  state=LauncherGuard(INFRA).status();assert state['phase']=='READY' and not state['inflight'] and not state['holds']
  with operation(INFRA,'amiberry-030','One exact master establishment fixture',may_leave_running=True):
   install_client();install_ipc(INFRA);guest=Guest(INFRA,HERE);client=AmiberryIPCClient(socket_path='/tmp/amiberry.sock')
   owned=processes();assert owned==[tuple(x) for x in started['ownedProcess']] and len(owned)==1 and owns(owned[0][1],'amiberry-030')
   def live():
    assert processes()==owned;target(INFRA,'amiberry-030')
   live();r['ownedProcess']=owned
   r['cpu']=await client.get_cpu_model();r['memory']=await client.get_memory_config();r['chipset']=await client.get_chipset();r['dmaBefore']=await client.get_dma_state();r['statusBefore']=guest.command('GET_STATUS')
   assert r['cpu']['model']=='68030' and str(r['cpu']['fpu'])=='0'
   assert r['memory']['chip']=='2048KB' and r['memory']['z3']=='131072KB' and r['memory']['rtg']=='0KB' and r['chipset'][1].upper()=='AGA'
   assert 'Paused=false' in r['statusBefore'] and all(r['dmaBefore']['audio'+str(i)]=='0' for i in range(4))
   binary=Path(product['path']).read_bytes();dest='Dev:Tests/'+STAGE.name
   launcher=('FailAt 21\nStack 65536\nCD '+dest+'\nPTMasterEstablishTest >fixture.log\nSet PTMasterResult $RC\nEcho $PTMasterResult >fixture.rc\nEcho COMPLETE >complete.flag\nQuit $PTMasterResult\n').encode('ascii')
   with transport(INFRA,'amiberry-030'):
    live();assert not STAGE.exists();STAGE.mkdir();(STAGE/'PTMasterEstablishTest').open('xb').write(binary);(STAGE/'Run-once').open('xb').write(launcher)
   r['candidate']=fp(STAGE/'PTMasterEstablishTest');r['launcher']=fp(STAGE/'Run-once')
   async with streamable_http_client('http://127.0.0.1:3000/mcp') as (rd,wr,_):
    async with ClientSession(rd,wr) as session:
     await session.initialize()
     async def call(name,args):
      live();return require_reply(await asyncio.wait_for(session.call_tool(name,args),20))
     r['pingBefore']=await call('amiga_ping',{})
     checksum(await call('amiga_checksum',{'path':dest+'/PTMasterEstablishTest'}),binary)
     checksum(await call('amiga_checksum',{'path':dest+'/Run-once'}),launcher)
     r['launches']=1
     r['launchReply']=await call('amiga_run_script',{'script':'Run >'+dest+'/launcher.log <NIL: Execute '+dest+'/Run-once','timeout':10})
     start=time.monotonic();deadline=start+60
     while not (STAGE/'complete.flag').exists() and time.monotonic()<deadline:await asyncio.sleep(.1)
     assert (STAGE/'complete.flag').exists(),'Exact fixture deadline; preserve resident failure'
     r['elapsedSeconds']=time.monotonic()-start
     r['downloadedLogs']={}
     for name in ('fixture.log','fixture.rc','complete.flag','launcher.log'):
      p=STAGE/name;assert p.is_file() and 0<p.stat().st_size<=1048576
      b=p.read_bytes();(HERE/name).open('xb').write(b)
      checksum(await call('amiga_checksum',{'path':dest+'/'+name}),b)
      r['downloadedLogs'][name]=fp(HERE/name)
     assert (HERE/'fixture.rc').read_bytes().strip()==b'0' and (HERE/'complete.flag').read_bytes().strip()==b'COMPLETE'
     text=(HERE/'fixture.log').read_text();assert 'MIXED OWNER PASS: full255' in text and 'ESTABLISH PASS:6 mono/stereo8/16/24' in text and ';6 owned-master reuse cases;81 cancellation/' in text and ';3 actual established mixed handoffs;' in text
     r['tasksAfter']=await call('amiga_run_script',{'script':'Status FULL','timeout':10})
     assert 'PTMasterEstablishTest' not in r['tasksAfter'] and '[OK]' in r['tasksAfter']
     r['dmaAfter']=await client.get_dma_state();assert all(r['dmaAfter']['audio'+str(i)]=='0' for i in range(4))
     live();r['status']='PASS_EXACT_030_SOFTWARE_FIXTURE_SETTLEMENT_PENDING'
 except BaseException as e:
  r.update(status='FIRST_FAILURE_HOLD_NO_RETRY',errorType=type(e).__name__,error=str(e)[:500])
  # Host custody only; do not send any additional target request or cleanup.
  for name in ('fixture.log','fixture.rc','complete.flag','launcher.log'):
   p=STAGE/name
   if p.is_file() and p.stat().st_size<=1048576 and not (HERE/('failure-'+name)).exists():(HERE/('failure-'+name)).write_bytes(p.read_bytes())
 finally:
  r['guardAfter']=LauncherGuard(INFRA).status();r['completedUTC']=datetime.datetime.now(datetime.timezone.utc).isoformat();save('run-terminal.json',r)
 print(json.dumps({k:r[k] for k in ('status','launches')}),flush=True)
 return 0 if r['status'].startswith('PASS_') else 20
with (INFRA/'runtime/test.lock').open('r+b') as f:
 fcntl.flock(f,fcntl.LOCK_EX|fcntl.LOCK_NB);sys.exit(asyncio.run(main()))
