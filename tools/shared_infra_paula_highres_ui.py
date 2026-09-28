#!/usr/bin/env python3
"""High-resolution Paula editor acceptance; requires a separately coordinated shared030 window."""
import argparse,fcntl,hashlib,json,re,shutil,subprocess,sys,time
from pathlib import Path
from shared_infra_render_files import prepare_run,require_running_guest,finish_run
from shared_infra_invert_ui import sample_records
ROOT=Path(__file__).resolve().parents[1]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('candidate',type=Path);parser.add_argument('fixture',type=Path);args=parser.parse_args()
    manifest=json.loads((args.candidate.parent/'core-build.json').read_text())
    assert digest(args.candidate)==manifest['binaries']['PT24GEdit']['sha256']
    sys.path.insert(0,str(INFRA/'scripts'));from shared_guest import Guest
    out=ROOT/'build/dev'/('paula-highres-ui-'+str(time.time_ns()));out.mkdir()
    result={'passed':False,'scope':'shared030 high-resolution Paula play/edit/undo/save/reopen; no physical acceptance'}
    with (INFRA/'runtime/test.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB);guest=Guest(INFRA,out);run=prepare_run(guest,out)
        finished=False;environment=False;active=None;start=time.monotonic()
        fixture=args.fixture;original=fixture.read_bytes()
        shutil.copyfile(args.candidate,run/'PT24GEdit');(run/'source.ptg').write_bytes(original);(run/'copies').mkdir()
        result.update(editor_sha256=digest(args.candidate),source_sha256=digest(fixture))
        device=guest.device+run.name
        settings={'PT24G_RECENT_PREFIX':device+'/recent','PT24G_RECOVERY_DIR':device+'/copies','PT24G_RECOVERY_MEDIA':'fixed','PT24G_RECOVERY_SECONDS':'300','PT24G_RECOVERY_REMOVABLE':'0'}
        def wait(predicate,seconds=20):
            deadline=min(start+180,time.monotonic()+seconds)
            while time.monotonic()<deadline:
                if predicate():return
                time.sleep(.1)
            raise RuntimeError('Paula UI deadline; preserve evidence and owned state')
        def text(name):return (run/name).read_text(errors='replace') if (run/name).exists() else ''
        def execute(script):
            reply=subprocess.run([str(INFRA/'.venv/bin/python'),str(INFRA/'scripts/mcp-call.py'),'amiga_run_script',json.dumps({'script':'Execute '+device+'/'+script,'timeout':5})],capture_output=True,text=True,timeout=20)
            (out/(script+'.log')).write_text(reply.stdout+reply.stderr)
            if reply.returncode or '[OK]' not in reply.stdout:raise RuntimeError('Environment script failed')
        setup=['FailAt 1','CD '+device];restore=['FailAt 1','CD '+device]
        for i,key in enumerate(settings):
            setup+=['If EXISTS ENV:'+key,'Copy ENV:'+key+' before-'+str(i),'EndIf']
        setup+=['Echo ready >backup-ready']
        for i,(key,value) in enumerate(settings.items()):
            setup+=['SetEnv '+key+' '+value,'Copy ENV:'+key+' active-'+str(i)]
            restore+=['If EXISTS before-'+str(i),'Copy before-'+str(i)+' ENV:'+key,'Else','If EXISTS ENV:'+key,'Delete ENV:'+key,'EndIf','EndIf','If EXISTS ENV:'+key,'Copy ENV:'+key+' after-'+str(i),'EndIf']
        setup+=['Echo ready >setup-done'];restore+=['Echo ready >restore-done']
        (run/'setup-env').write_text('\n'.join(setup)+'\n');(run/'restore-env').write_text('\n'.join(restore)+'\n')
        def launch(name,command):
            nonlocal active
            require_running_guest(guest,out,'before-'+name)
            guest.launch.write_text('\n'.join(['FailAt 21','Stack 65536','CD '+device,command+' >'+name+'.log','Echo $RC >'+name+'.rc','FailAt 1','CD RAM:','Echo done >'+device+'/'+name+'-done'])+'\n')
            active=name;guest.start()
        def completed(name):
            wait(lambda:(run/(name+'-done')).exists())
            assert text(name+'.rc').strip()=='0',text(name+'.log')
        def key(raw,control=False):
            if control:guest.command('SEND_KEY',0x63,1)
            try:guest.tap(raw)
            finally:
                if control:guest.command('SEND_KEY',0x63,0)
        def quit_editor(name):
            for _ in range(3):
                if (run/(name+'-done')).exists():break
                key(0x45);time.sleep(.6)
            completed(name);assert 'EDITOR EXIT clean' in text(name+'.log')
        def stop_editor(name):
            offset=len(text(name+'.log'));key(0x59)
            wait(lambda:any('EDITOR FRAME' in line and 'status=STOPPED - AUDIO RELEASED' in line for line in text(name+'.log')[offset:].splitlines()))
            state=guest.command('GET_AUDIO_STATE')
            assert all('ch%d_dma=0'%i in state.split('\t') for i in range(4)),state
        def guest_capture(name):
            reply=subprocess.run([str(INFRA/'.venv/bin/python'),str(INFRA/'scripts/mcp-call.py'),'amiga_screenshot','{}'],capture_output=True,text=True,timeout=20)
            (out/(name+'-guest.json')).write_text(reply.stdout+reply.stderr)
            if reply.returncode:raise RuntimeError('Guest bitmap capture failed')
            report=json.loads(reply.stdout);description=report['structuredContent']['result']
            match=re.fullmatch(r'Screenshot saved: (.+\.ppm) \(640x512, 4 planes, 2048 rows received\)',description)
            if not match:raise RuntimeError('Guest bitmap dimensions/completeness unverified')
            raw=Path(match.group(1)).read_bytes()
            if not raw.startswith(b'P6') or len(raw)>2*1024*1024:raise RuntimeError('Unexpected bitmap data')
            (out/(name+'-guest.ppm')).write_bytes(raw)
        try:
            execute('setup-env');environment=True
            assert (run/'setup-done').exists()
            for i,value in enumerate(settings.values()):assert (run/('active-'+str(i))).read_bytes()==value.encode()
            records=sample_records(original)
            assert len(records)==2 and [r[40] for r in records]==[24,16]
            launch('editor','PT24GEdit source.ptg saved.ptg')
            wait(lambda:'status=READY -' in text('editor.log'))
            def frame(fragment,offset=0):
                wait(lambda:any('EDITOR FRAME' in line and fragment in line for line in text('editor.log')[offset:].splitlines()))
            def audio(enabled):
                state=guest.command('GET_AUDIO_STATE')
                assert all('ch%d_dma=%d'%(i,enabled) in state.split('\t') for i in range(4)),state
                return state
            key(0x57)
            wait(lambda:'EDITOR REPLAY active=1' in text('editor.log') and 'period=428,339,285,214' in text('editor.log'))
            result['playing_audio']=audio(1);guest_capture('playing')
            # Reverse only sub8-bit values. Derived bytes are unchanged, but the
            # editor sample generation must still stop and invalidate playback.
            key(0x28,True);frame('panel=5')
            offset=len(text('editor.log'));key(0x13);frame('revision=1 dirty=1',offset)
            result['sample_edit_stopped_audio']=audio(0)
            offset=len(text('editor.log'));key(0x31,True);frame('revision=0 dirty=0',offset)
            key(0x45);frame('panel=0')
            offset=len(text('editor.log'));key(0x58)
            wait(lambda:'EDITOR REPLAY active=1' in text('editor.log')[offset:])
            result['restart_audio']=audio(1)
            key(0x21,True);wait(lambda:'EDITOR SAVE result=0 dirty=0' in text('editor.log'))
            assert (run/'saved.ptg').read_bytes()==original
            stop_editor('editor');quit_editor('editor')
            launch('reopened','PT24GEdit saved.ptg reopened.ptg')
            wait(lambda:'status=READY -' in text('reopened.log'))
            key(0x57);wait(lambda:'EDITOR REPLAY active=1' in text('reopened.log') and 'period=428,339,285,214' in text('reopened.log'))
            audio(1);key(0x21,True)
            wait(lambda:'EDITOR SAVE result=0 dirty=0' in text('reopened.log'))
            assert (run/'reopened.ptg').read_bytes()==original
            assert (run/'source.ptg').read_bytes()==original
            guest_capture('reopened');stop_editor('reopened');quit_editor('reopened')
            result.update(low_bit_sample_revision_stops=True,undo_exact=True,save_exact=True,reopen_exact=True,source_unchanged=True,normal_exit=True)
            execute('restore-env');environment=False;finished=True;result['passed']=True
        except Exception as error:
            result['error']=str(error)
            if active in ('editor','reopened') and not (run/(active+'-done')).exists():
                try:quit_editor(active)
                except Exception:pass
            if active is None or ((run/(active+'-done')).exists() and text(active+'.rc').strip()=='0'):
                if (run/'backup-ready').exists():
                    execute('restore-env');environment=False;finished=True
            raise
        finally:
            for p in run.iterdir():
                if p.is_file() and p.name not in ('PT24GEdit',):shutil.copyfile(p,out/p.name)
            if (run/'copies').exists():shutil.copytree(run/'copies',out/'copies')
            try:
                if finished:
                    restored=(run/'restore-done').exists() and not environment
                    for i in range(len(settings)):
                        before=run/('before-'+str(i));after=run/('after-'+str(i))
                        restored=restored and before.exists()==after.exists() and (not before.exists() or before.read_bytes()==after.read_bytes())
                    result['environment_restored']=restored
                    if not restored:finished=False;result['passed']=False;raise RuntimeError('Environment restoration unproved; keep hold')
            finally:finish_run(guest,run,out,result,finished,True)
if __name__=='__main__':main()
