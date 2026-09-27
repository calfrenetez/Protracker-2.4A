from pathlib import Path
import fcntl,hashlib,json,shutil,sys
infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
sys.path.insert(0,str(infra/'scripts'))
from shared_guest import Guest
out=Path(__file__).parent/'render-files-1790507331337658000'
expected='319fe845c737ad1f4545d4c02b5686f15d494b619f3665e0e70863026c0665a3'
with (infra/'runtime/test.lock').open('a') as lock:
 fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
 g=Guest(infra,out);run=g.share/out.name;sub=run/'editor-wavetable'
 assert run.is_dir() and not run.is_symlink() and not sub.is_symlink()
 assert sorted(p.name for p in run.iterdir())==['done','editor-wavetable']
 assert sorted(p.name for p in sub.iterdir())==['PTExecEditorWavetableTest','test.log','test.rc']
 assert (run/'done').read_text().strip()=='done'
 assert (sub/'test.rc').read_text().strip()=='0'
 assert hashlib.sha256((sub/'PTExecEditorWavetableTest').read_bytes()).hexdigest()==expected
 log=(sub/'test.log').read_text()
 for marker in ('NATIVE ALARM PASS:','NATIVE SIGNAL PASS:','NATIVE SONG GATE PASS:','NATIVE COST PASS:','PREPARED TRIGGER PASS:','EDITOR WAVETABLE PASS:','EXEC MEMORY PASS: 1241 Fast allocations, zero owned bytes, budget refusal without Chip fallback'):
  assert marker in log,marker
 assert log.rstrip().endswith('EXEC MEMORY PASS: 1241 Fast allocations, zero owned bytes, budget refusal without Chip fallback')
 launch=g.launch.read_text()
 assert 'PTExecEditorWavetableTest Dev:Tests/'+out.name+'/editor-wavetable >test.log' in launch
 assert launch.rstrip().endswith('Echo done >Dev:Tests/'+out.name+'/done')
 state=g.command('GET_AUDIO_STATE')
 assert all('ch%d_dma=0'%i in state.split('\t') for i in range(4)),state
 (out/'editor-wavetable.log').write_text(log)
 shutil.copy2(sub/'test.rc',out/'recovered.rc');shutil.copy2(run/'done',out/'recovered.done')
 (out/'recovered-launcher').write_text(launch)
 # Exact completed run only; preserve the original failed result.json.
 g.launch.unlink();shutil.rmtree(run)
 assert not run.exists() and not g.launch.exists()
 report={'original_deadline_passed':False,'completed_after_timeout':True,'returncode':0,'candidate_sha256':expected,'cleanup_audio':state,'run_files_cleaned':True,'scope':'late completion and guarded recovery only; not a90second test pass'}
 (out/'recovery.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
