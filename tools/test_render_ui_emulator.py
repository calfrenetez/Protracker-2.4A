#!/usr/bin/env python3
"""Native editor render settings, modal cancellation and exact WAV/PTG preservation. Reserve Amiberry first."""
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
    settings_only=sys.argv[1:]==['--settings-only']
    if sys.argv[1:] and not settings_only:raise SystemExit('Usage: test_render_ui_emulator.py [--settings-only]')
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    manifest=json.loads((ROOT/'build/dev/core-build.json').read_text())
    assert all(digest(ROOT/p)==h for p,h in manifest['sources'].items())
    for name in ['PT24GEdit','PT24GConvert','PTRenderFileTest']:
        assert digest(ROOT/'build/dev'/name)==manifest['binaries'][name]['sha256']
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('renderui'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/render-ui-evidence'/run.name;out.mkdir(parents=True)
    for name in ['PT24GEdit','PT24GConvert','PTRenderFileTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    fixture=ROOT/'evidence/enhanced-editor/dev28/native/speed.mod';shutil.copyfile(fixture,run/'input.mod')
    subprocess.run(['make','renderer'],cwd=ROOT,check=True)
    host=subprocess.check_output([str(ROOT/'build/host/PT24GRender'),str(fixture),str(out/'host.wav'),'--rate','44100','--bits','16','--gain','65536'],text=True)
    (out/'host.log').write_text(host)
    tuned=bytearray(fixture.read_bytes());assert tuned[44]==0;tuned[44]=1
    (out/'finetune.mod').write_bytes(tuned)
    tuned_log=subprocess.check_output([str(ROOT/'build/host/PT24GRender'),str(out/'finetune.mod'),str(out/'finetune-host.wav'),'--rate','44100','--bits','16','--gain','65536'],text=True)
    (out/'finetune-host.log').write_text(tuned_log)
    assert (out/'finetune-host.wav').read_bytes()!=(out/'host.wav').read_bytes()
    process=emu=None;start=time.monotonic()
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
        offset=key(0x11,ack=False);wait(lambda:'EDITOR REQUEST render' in log()[offset:]);time.sleep(.8);return offset
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
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTRenderFileTest . >files.log','Echo $RC >files.rc',
            'PT24GConvert project input.mod baseline.ptg >convert.log','Echo $RC >convert.rc',
            'PT24GEdit input.mod saved.ptg >editor.log','Echo $RC >editor.rc','Echo '+run.name+' >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['files','convert'])
        if settings_only:
            key(0x11,True,True);key(0x37)
            for _ in range(10):key(0x23)
            key(0x44);frame('INVALID TRACK MASK')
            screenshot=out/(run.name+'.png');time.sleep(.5);emu.command('SCREENSHOT',screenshot)
            wait(lambda:screenshot.exists() and screenshot.read_bytes().endswith(b'\0\0\0\0IEND\xaeB`\x82'));screenshot.rename(out/'render-settings.png')
            key(0x45);key(0x37);key(1);key(0x44);frame('RENDER TRACK MASK SET')
            key(0x21,True);frame('dirty=0 status=PROJECT SAVED')
            assert (run/'saved.ptg').read_bytes()==(run/'baseline.ptg').read_bytes()
            close_panel();key(0x45);wait(lambda:(run/'done').exists());assert (run/'editor.rc').read_text().strip()=='0'
        else:
            key(0x14,True,True);key(0x32);key(0x44);frame('revision=1 dirty=1 status=SONG TITLE UPDATED')
            key(0x57);key(0x11,True,True);key(0x13);key(0x35);key(0x24)
            screenshot=out/(run.name+'.png');time.sleep(.5);emu.command('SCREENSHOT',screenshot)
            wait(lambda:screenshot.exists() and screenshot.read_bytes().endswith(b'\0\0\0\0IEND\xaeB`\x82'));screenshot.rename(out/'render-settings.png')
            offset=request();key(0x45);frame('WAV REQUEST CANCELLED',offset);audio_off()
            offset=request();filename('cancel.wav');key(0x44,ack=False)
            wait(lambda:'progress=RENDERING' in log()[offset:]);key(0x45);frame('revision=1 dirty=1 status=WAV CANCELLED',offset)
            assert not (run/'cancel.wav').exists() and not list(run.glob('*.pttmp-*'));audio_off()
            offset=request();filename('render.wav');key(0x44,ack=False);frame('revision=1 dirty=1 status=WAV VERIFIED - PROJECT STILL UNSAVED',offset)
            assert (run/'render.wav').read_bytes()==(out/'host.wav').read_bytes();audio_off();print('PASS: cancelled mixing and exact dirty-project WAV',flush=True)
            key(0x31,True);frame('revision=0 dirty=0 status=UNDO');key(0x19)
            offset=request();filename('verifycancel.wav');key(0x44,ack=False)
            wait(lambda:'progress=VERIFYING percent=5' in log()[offset:]);key(0x45);frame('revision=0 dirty=0 status=WAV CANCELLED',offset)
            assert not (run/'verifycancel.wav').exists() and not list(run.glob('*.pttmp-*'))
            offset=request();filename('pattern.wav');key(0x44,ack=False);frame('revision=0 dirty=0 status=WAV VERIFIED - PROJECT UNCHANGED',offset)
            assert (run/'pattern.wav').read_bytes()==(run/'render.wav').read_bytes();audio_off()
            close_panel();key(0x4c,alt=True);frame('revision=2 dirty=1 status=')
            key(0x11,True,True);offset=request();filename('finetune.wav');key(0x44,ack=False)
            frame('revision=2 dirty=1 status=WAV VERIFIED - PROJECT STILL UNSAVED',offset)
            assert (run/'finetune.wav').read_bytes()==(out/'finetune-host.wav').read_bytes();audio_off()
            key(0x31,True);key(0x21,True);frame('dirty=0 status=PROJECT SAVED')
            assert (run/'saved.ptg').read_bytes()==(run/'baseline.ptg').read_bytes()
            close_panel();key(0x45);wait(lambda:(run/'done').exists());assert (run/'editor.rc').read_text().strip()=='0'
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PT24GConvert','PTRenderFileTest']},'fixture_sha256':digest(fixture),
            'settings_only':settings_only,
            'full_render_workflow_validated':not settings_only,'mask_overflow_refused':settings_only,'finetune_export_exact':not settings_only,
            'exact_project_identity':True,'stopped_audio':audio_off(),'editor_log':log(),'file_log':(run/'files.log').read_text(),
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        for n in (['baseline.ptg','saved.ptg'] if settings_only else ['render.wav','pattern.wav','finetune.wav','baseline.ptg','saved.ptg']):shutil.copyfile(run/n,out/n)
        (out/'native-render-ui.json').write_text(json.dumps(report,indent=2)+'\n')
        print('PASS: native '+('mask entry/overflow display and exact project preservation' if settings_only else 'render settings, request/mix/verify cancellation, song/pattern parity, finetune export and project preservation')+'; '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:
                    matches=matching_socket()
                    if len(matches)!=1:raise RuntimeError('No guarded socket; retain claim for recovery')
                    Emulator(matches[0]).command('QUIT')
            finally:process.wait(timeout=10)
        print('Render UI test released:',run,flush=True)
if __name__=='__main__':main()
