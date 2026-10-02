import fcntl,json,sys,time
from pathlib import Path
infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra');sys.path.insert(0,str(infra/'scripts'))
from shared_guest import Guest
from emulator import processes
root=Path(__file__).resolve().parents[2]
run=root/'build/dev/physical-amigus-ownership-1790982188208138000'
out=root/'build/dev'/('wavetable-physical-independent-'+str(time.time_ns()));out.mkdir()
result={'passed':False,'scope':'Subsequent independent original idle emulator and completed physical release evidence check'}
try:
 with (infra/'runtime/test.lock').open('a') as lock:
  fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
  prior=json.loads((run/'result.json').read_text());rows=processes();result['processes']=rows
  if [list(r) for r in rows]!=prior['emulator_process_before']:raise RuntimeError('Original emulator changed')
  if prior.get('passed') is not True or not all(prior.get(k) is True for k in ['run_files_cleaned','independent_cleanup_passed','execution_finished','devbench_returned']) or any(prior.get(k) for k in ['script_pending','ownership_release_pending','recovery_hold']):raise RuntimeError('Physical release/cleanup unconfirmed')
  if (run/'discovery.log').read_text().splitlines().count('WAVETABLE RELEASE confirmed=1 retained=0 driver=0x00000000')!=1:raise RuntimeError('Native own release proof missing')
  acknowledgements=[r['result'] for r in prior['calls'] if r['tool']=='amiga_run_script']
  if not any('PTG-CLEANUP-ABSENT' in r.splitlines() for r in acknowledgements) or not any('PTG-INDEPENDENT-ABSENCE' in r.splitlines() for r in acknowledgements):raise RuntimeError('Exact separate physical path absence proof missing')
  result['physical_directory']=prior['guest_directory'];result['physical_separate_absence_acknowledged']=True
  guest=Guest(infra,out);result['environment']=guest.env
  result['status']=guest.command('GET_STATUS');result['cpu']=guest.command('GET_CPU_MODEL');result['audio']=guest.command('GET_AUDIO_STATE')
  if [x for x in result['status'].split('\t') if x.startswith('Paused=')]!=['Paused=false'] or not all('ch%d_dma=0'%i in result['audio'].split('\t') for i in range(4)):raise RuntimeError('Independent idle guest guard failed')
  result['passed']=True
except Exception as error:
 result['error']=str(error);raise
finally:
 (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(out)
