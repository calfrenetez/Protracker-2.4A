import fcntl,json,os,sys,time
from pathlib import Path
infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra');sys.path.insert(0,str(infra/'scripts'))
from shared_guest import Guest
from emulator import processes
out=Path(__file__).resolve().parent/('wavetable-independent-'+str(time.time_ns()));out.mkdir()
result={'scope':'Independent subsequent read-only wavetable diagnostic cleanup verification','passed':False}
try:
 with (infra/'runtime/test.lock').open('a') as lock:
  fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
  rows=processes();result['processes']=rows
  if len(rows)!=1 or str(rows[0][0])!='79263':raise RuntimeError('Original emulator identity changed')
  guest=Guest(infra,out);result['environment']=guest.env
  result['status']=guest.command('GET_STATUS');result['cpu']=guest.command('GET_CPU_MODEL');result['audio']=guest.command('GET_AUDIO_STATE')
  result['paths']={str(guest.share/name):os.path.lexists(guest.share/name) for name in ('render-files-1790931158760994000','launch-render-files-1790931158760994000')}
  if any(result['paths'].values()) or [x for x in result['status'].split('\t') if x.startswith('Paused=')]!=['Paused=false'] or not all('ch%d_dma=0'%i in result['audio'].split('\t') for i in range(4)):raise RuntimeError('Independent cleanup verification failed')
  result['passed']=True
except Exception as error:
 result['error']=str(error);raise
finally:
 (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(out)
