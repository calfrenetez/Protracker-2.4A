from pathlib import Path
import fcntl,hashlib,json,sys,time
infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
sys.path.insert(0,str(infra/'scripts'))
from shared_guest import Guest
out=Path(__file__).parent/'render-files-1790508818178532000'
expected='ec2e3528a688184b23f3e3b0f9318bd4ba563aed64812b473e1bc77f07ed2ce9'
report={'scope':'explicitly user-approved resume of existing pending launch only; no reset/relaunch/cleanup'}
with (infra/'runtime/test.lock').open('a') as lock:
 fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
 g=Guest(infra,out);run=g.share/out.name;sub=run/'editor-wavetable'
 assert not run.is_symlink() and not sub.is_symlink() and not g.launch.is_symlink()
 assert hashlib.sha256((sub/'PTExecEditorWavetableTest').read_bytes()).hexdigest()==expected
 assert g.launch.read_bytes()==Path('evidence/enhanced-editor/prepared-restore/retained-launcher').read_bytes()
 report['mounts']=g.command('LIST_HARDDRIVES')
 assert report['mounts'].split('\t')==['OK','unit0=hdf:'+str(infra/'runtime/disks/Workbench-test.hdf'),'unit1=dir:'+str(infra/'runtime/Dev')]
 report['memory']=g.command('GET_MEMORY_CONFIG')
 assert 'chip=2048KB' in report['memory'].split('\t') and 'z3=131072KB' in report['memory'].split('\t')
 report['before']=g.command('GET_STATUS');assert 'Paused=true' in report['before'].split('\t')
 report['resume']=g.command('RESUME');report['after']=g.command('GET_STATUS')
 assert 'Paused=false' in report['after'].split('\t')
 (out/'resume-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
 print('Approved RESUME acknowledged; observing original pending launch only',flush=True)
 deadline=time.monotonic()+120
 while not (run/'done').exists() and time.monotonic()<deadline:time.sleep(.25)
 report['done']=(run/'done').exists()
 report['files']=[str(p.relative_to(run)) for p in run.rglob('*') if p.is_file()]
 report['status']=g.command('GET_STATUS');report['audio']=g.command('GET_AUDIO_STATE')
 if (sub/'test.rc').exists():report['rc']=(sub/'test.rc').read_text().strip()
 if (sub/'test.log').exists():(out/'resumed-test.log').write_bytes((sub/'test.log').read_bytes())
 (out/'resume-recovery.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
