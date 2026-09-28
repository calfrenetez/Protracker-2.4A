from pathlib import Path
import fcntl,hashlib,json,os,shutil,subprocess,sys,time
root=Path(__file__).resolve().parents[2]
infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
sys.path.insert(0,str(infra/'scripts'))
from shared_guest import Guest
candidate=root/'build/dev/recording-integration-candidate/build/dev'
manifest=json.loads((candidate/'core-build.json').read_text())
reserved=json.loads((candidate/'amigus-reservation-build.json').read_text())
manifest['binaries']['PTExecAmiGusReservedSessionTest']={'sha256':reserved['binary_sha256'],'bytes':reserved['binary_bytes']}
out=root/'build/dev/studio-streaming-qualification-v2';out.mkdir()
cases=[('queued-song',['PTExecQueuedSongTest','PTExecStudioPumpTest'],['--studio-memory','queued']),
       ('studio-consumer',['PTExecStudioConsumerTest','PTExecQueuedSongTest'],['--studio-memory','consumer']),
       ('editor-studio',['PTExecEditorStudioTest'],['--studio-memory','editor']),
       ('register-session',['PTExecAmiGusRegisterSessionTest'],['--studio-memory','register-session']),
       ('reserved-session',['PTExecAmiGusReservedSessionTest'],['--studio-memory','reserved-session'])]
# Pure host staging: no guest lease is acquired here.
for label,binaries,args in cases:
    for binary in binaries:
        src=candidate/binary
        assert hashlib.sha256(src.read_bytes()).hexdigest()==manifest['binaries'][binary]['sha256']
        shutil.copy2(src,root/'build/dev'/binary)
(out/'planned.json').write_text(json.dumps({'source_tree':(candidate.parent.parent/'source-tree.txt').read_text().strip(),'reserved_session_build':reserved,'cases':{binary:manifest['binaries'][binary] for label,binaries,args in cases for binary in binaries}},indent=2)+'\n')
start=time.monotonic();deadline=start+440;results=[]
for label,binaries,args in cases:
    if deadline-time.monotonic()<140:raise RuntimeError('Insufficient remaining window; no next guest launch')
    with (out/(label+'-runner.log')).open('w') as log:
        rc=subprocess.run([str(infra/'.venv/bin/python'),'tools/shared_infra_render_files.py',*args],cwd=root,stdout=log,stderr=subprocess.STDOUT,timeout=140).returncode
    raw=(out/(label+'-runner.log')).read_text()
    if rc:raise RuntimeError(label+' runner failed; stop sequence and inspect hold: '+raw)
    run=Path(raw.strip().splitlines()[-1]);result=json.loads((run/'result.json').read_text())
    assert result['passed'] and result['run_files_cleaned']
    for binary in binaries:
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
    results.append({'case':label,'binaries':{binary:result[binary+'_sha256'] for binary in binaries},'run':str(run),'independent_cleanup':True})
    (out/'result.json').write_text(json.dumps({'passed':len(results)==len(cases),'seconds':time.monotonic()-start,'window_bound_seconds':440,'cases':results},indent=2)+'\n')
    print(label,'PASS',run,flush=True)
print('ALL FIVE STUDIO STREAMING RUNS PASS; independent cleanup complete',flush=True)
