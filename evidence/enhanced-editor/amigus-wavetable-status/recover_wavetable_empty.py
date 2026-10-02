import fcntl,json,os,sys,time,stat
from pathlib import Path
infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra');sys.path.insert(0,str(infra/'scripts'))
from shared_guest import Guest
from emulator import processes
out=Path(__file__).resolve().parent/('wavetable-empty-recovery-'+str(time.time_ns()));out.mkdir()
result={'scope':'Separately scoped completed-run empty-directory recovery','passed':False}
try:
 with (infra/'runtime/test.lock').open('a') as lock:
  fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
  rows=processes();result['processes']=rows
  if len(rows)!=1 or str(rows[0][0])!='79263':raise RuntimeError('Original emulator identity changed')
  guest=Guest(infra,out);result['environment']=guest.env
  result['status']=guest.command('GET_STATUS');result['cpu']=guest.command('GET_CPU_MODEL');result['audio']=guest.command('GET_AUDIO_STATE')
  if [x for x in result['status'].split('\t') if x.startswith('Paused=')]!=['Paused=false'] or not all('ch%d_dma=0'%i in result['audio'].split('\t') for i in range(4)):raise RuntimeError('Unsafe recovery guest state')
  run=guest.share/'render-files-1790931158760994000'
  launcher=guest.share/'launch-render-files-1790931158760994000'
  expected={'ownership-fixture','driver-window','driver-exec','pcm-read','wavetable-read','native-abi','amigus-wavetable','amigus-discover','amigus-ownership','amigus-idle','amigus-registers','amigus-reset','amigus-fifo','amigus-capacity','amigus-capacity-observe'}
  if os.path.lexists(launcher) or not stat.S_ISDIR(run.lstat().st_mode) or {p.name for p in run.iterdir()}!=expected:raise RuntimeError('Exact recovery inventory changed')
  children=sorted(run.iterdir())
  if any(not stat.S_ISDIR(p.lstat().st_mode) or list(p.iterdir()) for p in children):raise RuntimeError('Recovery only permits expected empty nonsymlink directories')
  result['removed']=[]
  for p in children:
   p.rmdir();result['removed'].append(str(p))
  run.rmdir();result['removed'].append(str(run))
  result['absence_observations']=[]
  for i in range(11):
   observation={str(p):os.path.lexists(p) for p in (run,launcher)}
   result['absence_observations'].append(observation)
   if any(observation.values()):raise RuntimeError('Recovery path reappeared; no repeat deletion')
   if i<10:time.sleep(1)
  result['paths']={str(guest.share/name):os.path.lexists(guest.share/name) for name in ('render-files-1790931158760994000','launch-render-files-1790931158760994000')}
  if any(result['paths'].values()) or [x for x in result['status'].split('\t') if x.startswith('Paused=')]!=['Paused=false'] or not all('ch%d_dma=0'%i in result['audio'].split('\t') for i in range(4)):raise RuntimeError('Independent cleanup verification failed')
  result['passed']=True
except Exception as error:
 result['error']=str(error);raise
finally:
 (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(out)
