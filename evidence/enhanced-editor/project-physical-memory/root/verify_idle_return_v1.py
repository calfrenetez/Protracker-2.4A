#!/usr/bin/env python3
"""Separate locked read-only original030 idle verification after a scoped physical run."""
import argparse, asyncio, fcntl, hashlib, json, os, shlex, sys, time
from pathlib import Path
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
sys.path.insert(0,str(INFRA/'scripts'))
from shared_guest import Guest
from emulator import processes,owns
from bridge_checks import target
if not __debug__:raise RuntimeError('Optimized Python disables guards; refusing execution')
EXPECTED_PROFILE='894aa5a61a2bf6267b461c6774a0bdc913a96e7f581ea7e4e3762c4d71709a30'

def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def write(p,r):
 tmp=p.with_suffix('.pending');tmp.write_text(json.dumps(r,indent=2)+'\n');os.replace(tmp,p)
def guard(locks,original):
 for fd,p,identity in locks:
  s=os.fstat(fd);q=p.stat();assert (s.st_dev,s.st_ino)==(q.st_dev,q.st_ino)==identity,'Lock identity changed'
 rows=processes();assert rows==original and len(rows)==1 and owns(rows[0][1],'amiberry-030'),'Original process changed'
 words=shlex.split(rows[0][1]);profile=Path(words[words.index('-f')+1]);assert digest(profile)==EXPECTED_PROFILE,'Profile changed'
 target(INFRA,'amiberry-030');return rows
async def verify(out,original,locks,result):
 start=time.monotonic()
 for i in range(11):
  guard(locks,original);g=Guest(INFRA,out)
  cpu=g.command('GET_CPU_MODEL');status=g.command('GET_STATUS');audio=g.command('GET_AUDIO_STATE')
  sample={'elapsed_seconds':time.monotonic()-start,'processes':processes(),'cpu':cpu,'status':status,'audio':audio}
  result['observations'].append(sample);write(out/'result.json',result)
  assert 'model=68030' in cpu.split('\t') and 'Paused=false' in status.split('\t'),'Original guest identity/idle unknown'
  assert all('ch%d_dma=0'%n in audio.split('\t') for n in range(4)),'Original guest DMA not off'
  guard(locks,original)
  if i<10:await asyncio.sleep(1)
 assert result['observations'][-1]['elapsed_seconds']-result['observations'][0]['elapsed_seconds']>=10
 result['passed']=True

def main():
 p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
 a.out.mkdir(exist_ok=False);result={'passed':False,'scope':'Separate read-only original030 idle return; no physical reconnect, cleanup, fixture, audio or card operation','observations':[],'original_record_sha256':digest(a.original),'profile_sha256':EXPECTED_PROFILE}
 locks=[];handles=[];started=time.monotonic();write(a.out/'result.json',result)
 try:
  original=json.loads(a.original.read_text());assert len(original)==1 and original[0][0]==19081 and isinstance(original[0][1],str)
  original=[tuple(r) for r in original]
  for name in ('test.lock','real-a1200-safari.lock'):
   path=INFRA/'runtime'/name;assert path.is_file() and not path.is_symlink();fd=os.open(path,os.O_RDWR);handles.append(fd);fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);s=os.fstat(fd);locks.append((fd,path,(s.st_dev,s.st_ino)))
  result['locks']=[{'path':str(p),'identity':list(i)} for _,p,i in locks]
  asyncio.run(asyncio.wait_for(verify(a.out,original,locks,result),timeout=45))
 except BaseException as e:
  result['error']=str(e);result['recovery_hold']=bool(result['observations']);raise
 finally:
  result['elapsed_seconds']=time.monotonic()-started;write(a.out/'result.json',result)
  for fd in reversed(handles):os.close(fd)
  print(json.dumps({'out':str(a.out),'passed':result['passed'],'elapsed_seconds':result['elapsed_seconds'],'observations':len(result['observations'])}),flush=True)
if __name__=='__main__':main()
