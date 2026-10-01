from pathlib import Path
import fcntl,hashlib,json,os,shutil,subprocess,sys,time
root=Path(__file__).resolve().parents[2]
infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
sys.path.insert(0,str(infra/'scripts'))
from shared_guest import Guest
candidate=root/'build/dev/mixed-bulk-advance-candidate'
manifest=json.loads((candidate/'build/dev/core-build.json').read_text())
out=root/'build/dev'/('mixed-bulk-advance-qualification-'+str(time.time_ns()));out.mkdir()
cases=[('mixed-owner', 'PTExecMixedOwnerTest')]
for label,binary in cases:
    src=candidate/'build/dev'/binary
    assert hashlib.sha256(src.read_bytes()).hexdigest()==manifest['binaries'][binary]['sha256']
(out/'planned.json').write_text(json.dumps({'source_tree':(candidate/'source-tree.txt').read_text().strip(),'cases':{binary:manifest['binaries'][binary] for label,binary in cases}},indent=2)+'\n')
start=time.monotonic();deadline=start+200;results=[]
print('EVIDENCE',out,flush=True)
for label,binary in cases:
    if deadline-time.monotonic()<140:raise RuntimeError('Insufficient remaining window; no next guest launch')
    # Manifest-verified host copy into this indexed candidate only.
    with (out/(label+'-runner.log')).open('w') as log:
        rc=subprocess.run([str(infra/'.venv/bin/python'),'tools/shared_infra_render_files.py','--paula-memory',label],cwd=candidate,stdout=log,stderr=subprocess.STDOUT,timeout=140).returncode
    raw=(out/(label+'-runner.log')).read_text()
    if rc:raise RuntimeError(label+' runner failed; stop and inspect owned resources: '+raw)
    run=Path(raw.strip().splitlines()[-1]);result=json.loads((run/'result.json').read_text())
    assert result['passed'] and result['run_files_cleaned']
    assert result[binary+'_sha256']==manifest['binaries'][binary]['sha256']
    with (infra/'runtime/test.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB);guest=Guest(infra,run)
        status=guest.command('GET_STATUS');audio=guest.command('GET_AUDIO_STATE')
        assert 'Paused=false' in status.split('\t') and all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        assert not os.path.lexists(guest.share/run.name) and not os.path.lexists(guest.launch)
        clean={'status':status,'audio':audio,'owned_run_absent':True,'launcher_absent':True}
        (run/'independent-cleanup.json').write_text(json.dumps(clean,indent=2)+'\n')
    results.append({'case':label,'binary':binary,'run':str(run),'sha256':result[binary+'_sha256'],'independent_cleanup':True})
    (out/'result.json').write_text(json.dumps({'passed':len(results)==len(cases),'seconds':time.monotonic()-start,'window_bound_seconds':200,'cases':results},indent=2)+'\n')
    print(label,'PASS',run,flush=True)
print('MIXED OWNERSHIP FIXTURE PASS; independent cleanup complete',flush=True)
