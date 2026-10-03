import fcntl,hashlib,json,os,re,stat,sys,time
from pathlib import Path
assert __debug__
root=Path('/private/tmp/protracker-mod-current-root-1cn1g3y0');infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra');repo=Path('/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A')
execution=json.loads((root/'native-execution-next.json').read_bytes());assert execution['passed']
qualification=Path((root/'native-execution-next.log').read_text().strip().splitlines()[-1]);assert qualification.parent==repo/'build/dev' and re.fullmatch(r'mod-output-safety-current-qualification-[0-9]+',qualification.name)
d=json.loads((qualification/'qualification-result.json').read_bytes());assert d['passed'] and len(d['cases'])==1
run=Path(d['cases'][0]['run']);assert run.parent==qualification/'runner-mod-stream/build/dev' and re.fullmatch(r'render-files-[0-9]+',run.name)
identity=json.loads((root/'fresh-identity-next.json').read_bytes());sys.path.insert(0,str(infra/'scripts'))
from shared_guest import Guest
from emulator import processes
claim=root/'separate-idle-claim.json';fd=os.open(claim,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
with os.fdopen(fd,'w') as f:json.dump({'scope':'Separate read-only native absence/idle verification','claimed_ns':time.time_ns()},f)
report={'scope':'Separate locked read-only original emulator idle and exact path absence; no native rerun/cleanup/release','passed':False,'observations':[]};started=time.monotonic()
def save():report['elapsed_s']=time.monotonic()-started;(root/'separate-idle-result.json').write_text(json.dumps(report,indent=2)+'\n')
def original():
 rows=[list(x) for x in processes()];assert rows==[identity['process']]
 assert hashlib.sha256(Path(identity['profile']).read_bytes()).hexdigest()==identity['profile_sha256'];return rows
try:
 fd=os.open(infra/'runtime/test.lock',os.O_RDWR|os.O_NOFOLLOW)
 with os.fdopen(fd,'r+') as lock:
  assert stat.S_ISREG(os.fstat(lock.fileno()).st_mode);fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
  report['initial_processes']=original();guest=Guest(infra,run)
  def idle():
   status=guest.command('GET_STATUS');audio=guest.command('GET_AUDIO_STATE');cpu=guest.command('GET_CPU_MODEL')
   assert [s for s in status.split('\t') if s.startswith('Paused=')]==['Paused=false'] and 'model=68030' in cpu.split('\t') and all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
   return {'status':status,'audio':audio,'cpu':cpu}
  report['initial_idle']=idle();save()
  for i in range(11):
   paths={str(p):os.path.lexists(p) for p in [guest.share/run.name,guest.launch]};report['observations'].append({'elapsed_s':time.monotonic()-started,'paths':paths});save();assert not any(paths.values())
   if i<10:time.sleep(1.001)
  report['observation_span_s']=report['observations'][-1]['elapsed_s']-report['observations'][0]['elapsed_s'];assert report['observation_span_s']>10
  report['final_processes']=original();report['final_idle']=idle();report['passed']=True
except Exception as e:report['error']=str(e);raise
finally:save();print(json.dumps({'passed':report['passed'],'elapsed_s':report['elapsed_s'],'observations':len(report['observations'])}))
