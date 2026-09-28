#!/usr/bin/env python3
"""Recovery UI acceptance; requires a separately coordinated shared030 window."""
import argparse,fcntl,hashlib,json,re,shutil,subprocess,sys,time
from pathlib import Path
from shared_infra_render_files import prepare_run,require_running_guest,finish_run
from shared_infra_invert_ui import sample_records
ROOT=Path(__file__).resolve().parents[1]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('candidate',type=Path);parser.add_argument('--inspect-prompts',action='store_true');args=parser.parse_args()
    manifest=json.loads((args.candidate.parent/'core-build.json').read_text())
    assert digest(args.candidate)==manifest['binaries']['PT24GEdit']['sha256']
    seed=ROOT/'build/dev/PTExecRecoveryTest';seed_manifest=json.loads((seed.parent/'recovery-build.json').read_text())
    assert digest(seed)==seed_manifest['binary_sha256']
    sys.path.insert(0,str(INFRA/'scripts'));from shared_guest import Guest
    out=ROOT/'build/dev'/('recovery-ui-'+str(time.time_ns()));out.mkdir()
    result={'passed':False,'scope':'shared030 recovery offer, decline, restore, idle autosave and exact master save; no physical acceptance'}
    with (INFRA/'runtime/test.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB);guest=Guest(INFRA,out);run=prepare_run(guest,out)
        finished=False;environment=False;active=None;start=time.monotonic()
        fixture=ROOT/'tests/fixtures/project-v1/mixed.ptg';original=fixture.read_bytes()
        shutil.copyfile(args.candidate,run/'PT24GEdit');shutil.copyfile(seed,run/'PTExecRecoveryTest');(run/'source.ptg').write_bytes(original);(run/'copies').mkdir()
        result.update(editor_sha256=digest(args.candidate),seed_sha256=digest(seed),source_sha256=digest(fixture))
        device=guest.device+run.name
        settings={'PT24G_RECENT_PREFIX':device+'/recent','PT24G_RECOVERY_DIR':device+'/copies','PT24G_RECOVERY_MEDIA':'fixed','PT24G_RECOVERY_SECONDS':'30','PT24G_RECOVERY_REMOVABLE':'0'}
        def wait(predicate,seconds=20):
            deadline=min(start+240,time.monotonic()+seconds)
            while time.monotonic()<deadline:
                if predicate():return
                time.sleep(.1)
            raise RuntimeError('Recovery UI deadline; preserve evidence and owned state')
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
        def capture(name):
            target=out/name;guest.command('SCREENSHOT',target)
            wait(lambda:target.exists() and target.read_bytes().endswith(b'\0\0\0\0IEND\xaeB`\x82'))
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
        def inspect_prompt(name,marker):
            if not args.inspect_prompts:return
            guest_capture(name)
            deadline=time.monotonic()+45;number=0
            while not (out/marker).exists():
                if time.monotonic()>deadline:raise RuntimeError('Prompt visual inspection deadline')
                number+=1;capture(name+'-%02d.png'%number);time.sleep(2)
        try:
            execute('setup-env');environment=True
            assert (run/'setup-done').exists()
            for i,value in enumerate(settings.values()):assert (run/('active-'+str(i))).read_bytes()==value.encode()
            launch('seed','PTExecRecoveryTest --seed '+device+'/source.ptg '+device+'/copies '+device+'/expected.ptg')
            completed('seed');assert 'NATIVE RECOVERY SEED PASS:' in text('seed.log') and 'EXEC MEMORY PASS:' in text('seed.log')
            assert (run/'source.ptg').read_bytes()==original
            expected=(run/'expected.ptg').read_bytes();assert sample_records(expected)==sample_records(original)
            initial={p.name for p in (run/'copies').iterdir()};assert len(initial)==1
            launch('decline','PT24GEdit source.ptg declined.ptg')
            wait(lambda:'EDITOR RECOVERY offered' in text('decline.log'));time.sleep(1);capture('recovery-offer.png')
            inspect_prompt('offer-live','offer-inspected')
            key(0x45)
            wait(lambda:'EDITOR FRAME' in text('decline.log'))
            assert 'EDITOR RECOVERY declined' in text('decline.log') and 'dirty=0' in text('decline.log')
            quit_editor('decline');assert not (run/'declined.ptg').exists()
            assert {p.name for p in (run/'copies').iterdir()}==initial
            result['decline_preserved_project_and_copy']=True
            launch('restore','PT24GEdit source.ptg saved.ptg')
            wait(lambda:'EDITOR RECOVERY offered' in text('restore.log'));time.sleep(1)
            capture('recovery-confirm.png')
            inspect_prompt('confirm-live','confirm-inspected')
            # Intuition EasyRequest uses Left-Amiga+V for its leftmost gadget,
            # not Return: https://wiki.amigaos.net/wiki/Intuition_Requesters
            guest.command('SEND_KEY',0x66,1)
            try:key(0x34)
            finally:guest.command('SEND_KEY',0x66,0)
            wait(lambda:'EDITOR RECOVERY restored dirty=1' in text('restore.log'))
            wait(lambda:'EDITOR FRAME' in text('restore.log'));capture('recovered-editor.png');guest_capture('recovered-editor')
            wait(lambda:'EDITOR RECOVERY snapshot=1 dirty=1' in text('restore.log'),45)
            assert len(list((run/'copies').iterdir()))==2
            result['idle_autosave']=True
            key(0x21,True);wait(lambda:'EDITOR SAVE result=0 dirty=0' in text('restore.log'))
            assert (run/'saved.ptg').read_bytes()==expected
            assert {p.name for p in (run/'copies').iterdir()}==initial
            assert (run/'source.ptg').read_bytes()==original
            capture('recovered-saved.png');guest_capture('recovered-saved');quit_editor('restore')
            result.update(exact_restored_project=True,master_records_unchanged=True,original_unchanged=True,clean_save_discards_owned_snapshot=True,normal_exit=True)
            execute('restore-env');environment=False;finished=True;result['passed']=True
        except Exception as error:
            result['error']=str(error)
            if active in ('decline','restore') and not (run/(active+'-done')).exists():
                try:quit_editor(active)
                except Exception:pass
            if active is None or ((run/(active+'-done')).exists() and text(active+'.rc').strip()=='0'):
                if (run/'backup-ready').exists():
                    execute('restore-env');environment=False;finished=True
            raise
        finally:
            for p in run.iterdir():
                if p.is_file() and p.name not in ('PT24GEdit','PTExecRecoveryTest'):shutil.copyfile(p,out/p.name)
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
