from pathlib import Path
import fcntl,hashlib,json,shutil,sys,re
infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra');sys.path.insert(0,str(infra/'scripts'))
from shared_guest import Guest
out=Path(__file__).parent/'render-files-1790508818178532000'
expected='ec2e3528a688184b23f3e3b0f9318bd4ba563aed64812b473e1bc77f07ed2ce9'
with (infra/'runtime/test.lock').open('a') as lock:
 fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB);g=Guest(infra,out);run=g.share/out.name;sub=run/'editor-wavetable'
 assert run.is_dir() and not run.is_symlink() and not sub.is_symlink() and not g.launch.is_symlink()
 assert sorted(p.name for p in run.iterdir())==['done','editor-wavetable']
 assert sorted(p.name for p in sub.iterdir())==['PTExecEditorWavetableTest','test.log','test.rc']
 assert (run/'done').read_text().strip()=='done' and (sub/'test.rc').read_text().strip()=='0'
 assert hashlib.sha256((sub/'PTExecEditorWavetableTest').read_bytes()).hexdigest()==expected
 assert g.launch.read_bytes()==Path('evidence/enhanced-editor/prepared-restore/retained-launcher').read_bytes()
 log=(sub/'test.log').read_text()
 for marker in ('NATIVE ECLOCK PASS:','NATIVE ALARM PASS:','NATIVE SIGNAL PASS:','NATIVE SONG GATE PASS:','NATIVE COST PASS:','WAVETABLE STAGED RESTORE PASS:','PREPARED BATCH PASS:','EDITOR WAVETABLE PASS:'):
  assert marker in log,marker
 final=re.search(r'EXEC MEMORY PASS: (\d+) Fast allocations, zero owned bytes, budget refusal without Chip fallback\s*$',log);assert final
 status=g.command('GET_STATUS');assert 'Paused=false' in status.split('\t')
 audio=g.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
 (out/'recovered-test.log').write_text(log)
 for src,dst in [(sub/'test.rc','recovered.rc'),(run/'done','recovered.done'),(g.launch,'recovered-launcher')]:shutil.copyfile(src,out/dst)
 # Preserve original failed launch report. Delete only the exact completed owned run.
 g.launch.unlink();shutil.rmtree(run);assert not run.exists() and not g.launch.exists()
 result={'original_launch_passed':False,'user_approved_resume':True,'completed_original_pending_launch':True,'returncode':0,'candidate_sha256':expected,'fast_allocations':int(final.group(1)),'status':status,'cleanup_audio':audio,'run_files_cleaned':True,'scope':'recovered native assertions and exact cleanup; not a successful original launch or hardware/realtime acceptance'}
 (out/'recovery.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
