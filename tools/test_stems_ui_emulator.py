#!/usr/bin/env python3
"""Native editor stem export, cancellation and exact WAV/PTG preservation. Reserve Amiberry first."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket


def main():
    ram=sys.argv[1:]==['--ram']
    if sys.argv[1:] and not ram:raise SystemExit('Usage: test_stems_ui_emulator.py [--ram]')
    processes=subprocess.check_output(['ps','ax','-o','pid=,comm='],text=True)
    assert not any(line.lower().endswith('/amiberry') for line in processes.splitlines())
    holders=subprocess.run(['lsof',str(ROOT/'local/baseline.hdf')],capture_output=True,text=True)
    assert holders.returncode==1 and not holders.stdout
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('stemsui'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/stems-ui-evidence'/run.name;out.mkdir(parents=True)
    for name in ['PT24GEdit','PT24GConvert']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    fixture=ROOT/'evidence/enhanced-editor/dev48/native/fine_00.mod';shutil.copyfile(fixture,run/'input.mod')
    subprocess.run(['make','renderer'],cwd=ROOT,check=True)
    host=subprocess.check_output([str(ROOT/'build/host/PT24GRender'),str(fixture),str(out/'host'),'--groups'],text=True)
    (out/'host.log').write_text(host)
    process=emu=None;start=time.monotonic();observations=[]
    def observe(stage):
        observations.append({'stage':stage,'elapsed':round(time.monotonic()-start,3),'staging':[str(p.relative_to(run)) for p in run.rglob('*') if '.ptstems-' in str(p)]})
        (out/'host-observations.json').write_text(json.dumps(observations,indent=2)+'\n')
    def log():return (run/'editor.log').read_text() if (run/'editor.log').exists() else ''
    def wait(check,seconds=180):
        end=min(time.monotonic()+seconds,start+600)
        while time.monotonic()<end:
            if check():return
            if process.poll() is not None:raise RuntimeError('Emulator exited: '+str(run))
            time.sleep(.1)
        raise RuntimeError('Render UI deadline '+str(run)+' '+log()[-700:])
    def frame(text,after=0):wait(lambda:any('EDITOR FRAME' in s and text in s for s in log()[after:].splitlines()))
    def key(raw,control=False,shift=False,alt=False,ack=True):
        offset=len(log())
        if control:emu.command('SEND_KEY',0x63,1)
        if shift:emu.command('SEND_KEY',0x60,1)
        if alt:emu.command('SEND_KEY',0x64,1)
        try:emu.tap(raw)
        finally:
            if alt:emu.command('SEND_KEY',0x64,0)
            if shift:emu.command('SEND_KEY',0x60,0)
            if control:emu.command('SEND_KEY',0x63,0)
        if ack:wait(lambda:'EDITOR FRAME' in log()[offset:] or 'EDITOR EXIT clean' in log()[offset:])
        return offset
    def request():
        offset=key(0x21,ack=False);wait(lambda:'EDITOR REQUEST stems' in log()[offset:]);time.sleep(.8);return offset
    def filename(text):
        keys=dict(zip('abcdefghijklmnopqrstuvwxyz',[0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31]));keys['.']=0x39
        emu.command('SEND_KEY',0x60,1)
        try:emu.tap(0x4f);emu.tap(0x46)
        finally:emu.command('SEND_KEY',0x60,0)
        for ch in text:emu.tap(keys[ch])
    def audio_off():
        state=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in state.split('\t') for i in range(4));return state
    def close_panel():
        key(0x45);key(0x12,True);offset=key(0x12,True);frame('panel=0',offset)
    try:
        script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PT24GConvert project input.mod baseline.ptg >convert.log','Echo $RC >convert.rc']
        if ram:
            script += ['MakeDir RAM:'+run.name,'Copy input.mod RAM:'+run.name+'/input.mod','CD RAM:'+run.name,
                'PTDEV:'+run.name+'/PT24GEdit input.mod PTDEV:'+run.name+'/saved.ptg >PTDEV:'+run.name+'/editor.log',
                'Echo $RC >PTDEV:'+run.name+'/editor.rc','List RAM:'+run.name+' ALL >PTDEV:'+run.name+'/ram-list.log',
                'Copy RAM:'+run.name+'/stems PTDEV:'+run.name+'/stems ALL',
                'Echo $RC >PTDEV:'+run.name+'/copy.rc',
                'Echo '+run.name+' >PTDEV:'+run.name+'/done']
        else:script += ['PT24GEdit input.mod saved.ptg >editor.log','Echo $RC >editor.rc','Echo '+run.name+' >done']
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['convert'])
        key(0x14,True,True);key(0x32);key(0x44);frame('revision=1 dirty=1 status=SONG TITLE UPDATED')
        key(0x11,True,True);key(0x18)
        screenshot=out/(run.name+'.png');time.sleep(.5);emu.command('SCREENSHOT',screenshot)
        wait(lambda:screenshot.exists() and screenshot.read_bytes().endswith(b'\0\0\0\0IEND\xaeB`\x82'));screenshot.rename(out/'stem-settings.png')
        offset=request();key(0x45);frame('STEMS REQUEST CANCELLED',offset);audio_off()
        offset=request();filename('cancel');key(0x44,ack=False)
        wait(lambda:'progress=RENDERING' in log()[offset:]);key(0x45);frame('revision=1 dirty=1 status=STEMS CANCELLED',offset)
        observe('after-cancel');
        if not ram:assert not (run/'cancel').exists() and not list(run.glob('*.ptstems-*'))
        audio_off()
        offset=request();observe('next-requester-open');filename('stems');key(0x44,ack=False);frame('revision=1 dirty=1 status=4 STEMS VERIFIED - PROJECT STILL UNSAVED',offset)
        key(0x31,True);frame('revision=0 dirty=0 status=UNDO');key(0x21,True);frame('dirty=0 status=PROJECT SAVED')
        assert (run/'saved.ptg').read_bytes()==(run/'baseline.ptg').read_bytes()
        close_panel();key(0x45);wait(lambda:(run/'done').exists());assert (run/'editor.rc').read_text().strip()=='0'
        if ram:
            assert (run/'copy.rc').read_text().strip()=='0'
            listing=(run/'ram-list.log').read_text();(out/'ram-list.log').write_text(listing)
            assert '.ptstems-' not in listing and '.pttmp-' not in listing and 'cancel' not in listing.lower(),listing
        assert sorted(p.name for p in (run/'stems').iterdir())==sorted(p.name for p in (out/'host').iterdir())
        for p in (out/'host').iterdir():assert p.read_bytes()==(run/'stems'/p.name).read_bytes()
        observe('after-export');assert not list(run.glob('*.ptstems-*'));audio_off()
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PT24GConvert']},'fixture_sha256':digest(fixture),
            'output_volume':'RAM:' if ram else 'PTDEV: shared host folder','exact_project_identity':True,'stopped_audio':audio_off(),'editor_log':log(),
            'scope':'Stem panel/group toggle, requester and mixing cancellation, exact four-stem host WAV parity, dirty-state preservation and exact PTG after undo/save. No hardware sound claim.',
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        shutil.copytree(run/'stems',out/'native')
        for n in ['baseline.ptg','saved.ptg']:shutil.copyfile(run/n,out/n)
        (out/'native-stems-ui.json').write_text(json.dumps(report,indent=2)+'\n')
        print('PASS: native stem editor workflow and exact WAV/PTG preservation; '+str(out),flush=True)
    finally:
        (out/"editor-capture.log").write_text(log())
        observe("before-guarded-cleanup")
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Render UI test released:',run,flush=True)
if __name__=='__main__':main()
