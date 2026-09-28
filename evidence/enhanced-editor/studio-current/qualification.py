from pathlib import Path
import fcntl,hashlib,json,os,shutil,subprocess,sys,time
root=Path(__file__).resolve().parents[2]
infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
sys.path.insert(0,str(infra/'scripts'))
from shared_guest import Guest
candidate=root/'build/dev/recording-integration-candidate/build/dev'
manifest=json.loads((candidate/'core-build.json').read_text())
out=root/'build/dev/studio-core-qualification';out.mkdir()
cases=[('studio','PTExecStudioTest',['--studio-memory','mixer']),
       ('sampler-studio','PTExecSamplerStudioTest',['--studio-memory','sampler']),
       ('amigus-session','PTExecAmiGusSessionTest',['--studio-memory','session']),
       ('amigus-chain','PTExecAmiGusChainTest',['--studio-memory','fifo']),
       ('studio-song','PTExecStudioSongTest',['--studio-memory','song'])]
# Pure host staging: no guest lease is acquired here.
for label,binary,args in cases:
    src=candidate/binary
    assert hashlib.sha256(src.read_bytes()).hexdigest()==manifest['binaries'][binary]['sha256']
    shutil.copy2(src,root/'build/dev'/binary)
(out/'planned.json').write_text(json.dumps({'source_tree':(candidate.parent.parent/'source-tree.txt').read_text().strip(),'cases':{binary:manifest['binaries'][binary] for label,binary,args in cases}},indent=2)+'\n')
start=time.monotonic();deadline=start+440;results=[]
for label,binary,args in cases:
    if deadline-time.monotonic()<140:raise RuntimeError('Insufficient remaining window; no next guest launch')
    with (out/(label+'-runner.log')).open('w') as log:
        rc=subprocess.run([str(infra/'.venv/bin/python'),'tools/shared_infra_render_files.py',*args],cwd=root,stdout=log,stderr=subprocess.STDOUT,timeout=140).returncode
    raw=(out/(label+'-runner.log')).read_text()
    if rc:raise RuntimeError(label+' runner failed; stop sequence and inspect hold: '+raw)
    run=Path(raw.strip().splitlines()[-1]);result=json.loads((run/'result.json').read_text())
    assert result['passed'] and result['run_files_cleaned']
    assert result[binary+'_sha256']==manifest['binaries'][binary]['sha256']
    # Separate subsequent lock and identity check: never infer cleanup solely
    # from the runner's success marker, and never retry/reset an uncertain run.
    with (infra/'runtime/test.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB);guest=Guest(infra,run)
        status=guest.command('GET_STATUS');audio=guest.command('GET_AUDIO_STATE')
        assert 'Paused=false' in status.split('\t') and all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        assert not os.path.lexists(guest.share/run.name) and not os.path.lexists(guest.launch)
        clean={'status':status,'audio':audio,'owned_run_absent':True,'launcher_absent':True}
        (run/'independent-cleanup.json').write_text(json.dumps(clean,indent=2)+'\n')
    results.append({'case':label,'binary':binary,'run':str(run),'sha256':result[binary+'_sha256'],'independent_cleanup':True})
    (out/'result.json').write_text(json.dumps({'passed':len(results)==len(cases),'seconds':time.monotonic()-start,'window_bound_seconds':440,'cases':results},indent=2)+'\n')
    print(label,'PASS',run,flush=True)
print('ALL FIVE STUDIO SOFTWARE FIXTURES PASS; independent cleanup complete',flush=True)
