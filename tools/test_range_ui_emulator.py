#!/usr/bin/env python3
"""Native marked-row WAV/stems/bounce with shared undo and persistence. Reserve Amiberry first."""
import json
from pathlib import Path
import shutil
import subprocess
import time
import zlib
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket


def main():
    processes=subprocess.check_output(['ps','ax','-o','pid=,comm='],text=True)
    assert not any(line.lower().endswith('/amiberry') for line in processes.splitlines())
    holders=subprocess.run(['lsof',str(ROOT/'local/baseline.hdf')],capture_output=True,text=True)
    assert holders.returncode==1 and not holders.stdout
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('rangeui'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/range-ui-evidence'/run.name;out.mkdir(parents=True)
    for name in ['PT24GEdit','PT24GConvert']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    fixture=ROOT/'evidence/enhanced-editor/dev48/native/fine_override.mod';shutil.copyfile(fixture,run/'input.mod')
    subprocess.run(['make','renderer'],cwd=ROOT,check=True)
    host=subprocess.check_output([str(ROOT/'build/host/PT24GRender'),str(fixture),str(out/'host.wav'),'--pattern','0','--from-row','1','--to-row','3','--tracks','1'],text=True)
    (out/'host.log').write_text(host)
    process=emu=None;start=time.monotonic();current='editor.log'
    def log():return (run/current).read_text() if (run/current).exists() else ''
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
    def chunks(data):
        check=bytearray(data);check[20:24]=bytes(4);assert zlib.crc32(check)==int.from_bytes(data[20:24],'big')
        result={};pos=32
        for _ in range(int.from_bytes(data[24:28],'big')):
            size=int.from_bytes(data[pos+8:pos+12],'big');result[data[pos:pos+4]]=data[pos+12:pos+12+size];pos+=12+size+(-size%4)
        assert pos==len(data);return result
    try:
        script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PT24GConvert project input.mod baseline.ptg >convert.log','Echo $RC >convert.rc',
            'MakeDir RAM:'+run.name,'Copy input.mod RAM:'+run.name+'/input.mod','CD RAM:'+run.name,
            'PTDEV:'+run.name+'/PT24GEdit input.mod PTDEV:'+run.name+'/saved.ptg >PTDEV:'+run.name+'/editor.log',
            'Echo $RC >PTDEV:'+run.name+'/editor.rc',
            'PTDEV:'+run.name+'/PT24GEdit PTDEV:'+run.name+'/saved.ptg PTDEV:'+run.name+'/reopened.ptg >PTDEV:'+run.name+'/reopened.log',
            'Echo $RC >PTDEV:'+run.name+'/reopened.rc',
            'List RAM:'+run.name+' ALL >PTDEV:'+run.name+'/ram-list.log',
            'Copy excerpt.wav PTDEV:'+run.name+'/excerpt.wav','Echo $RC >PTDEV:'+run.name+'/wav-copy.rc',
            'Copy stems PTDEV:'+run.name+'/stems ALL','Echo $RC >PTDEV:'+run.name+'/stem-copy.rc',
            'Echo '+run.name+' >PTDEV:'+run.name+'/done']
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['convert'])
        key(0x4d);key(0x35,True);key(0x4d);key(0x11,True,True);key(0x12)
        frame('MARKED ROWS READY')
        screenshot=out/(run.name+'.png');time.sleep(.5);emu.command('SCREENSHOT',screenshot)
        wait(lambda:screenshot.exists() and screenshot.read_bytes().endswith(b'\0\0\0\0IEND\xaeB`\x82'));screenshot.rename(out/'range-settings.png')
        offset=key(0x11,ack=False);wait(lambda:'EDITOR REQUEST render' in log()[offset:]);time.sleep(.8);filename('excerpt.wav');key(0x44,ack=False)
        frame('revision=0 dirty=0 status=WAV VERIFIED - PROJECT UNCHANGED',offset);audio_off()
        offset=key(0x21,ack=False);wait(lambda:'EDITOR REQUEST stems' in log()[offset:]);time.sleep(.8);filename('stems');key(0x44,ack=False)
        frame('revision=0 dirty=0 status=1 STEMS VERIFIED - PROJECT UNCHANGED',offset);audio_off()
        offset=key(0x16,ack=False);wait(lambda:'progress=RENDERING' in log()[offset:]);key(0x45)
        frame('revision=0 dirty=0 status=BOUNCE CANCELLED',offset);audio_off()
        assert 'samples=31 frames=0' in log()[offset:]
        offset=key(0x16,ack=False);frame('revision=1 dirty=1 status=BOUNCED TO SAMPLE 032',offset);audio_off()
        key(0x31,True);frame('revision=0 dirty=0 status=UNDO')
        # A failed new bounce must preserve the redo branch containing the completed one.
        offset=key(0x16,ack=False);wait(lambda:'progress=RENDERING' in log()[offset:]);key(0x45)
        frame('revision=0 dirty=0 status=BOUNCE CANCELLED',offset)
        key(0x31,True,True);frame('revision=1 dirty=1 status=REDO')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');audio_off()
        base=chunks((run/'baseline.ptg').read_bytes());actual=chunks((run/'saved.ptg').read_bytes());expected=dict(base)
        head=bytearray(base[b'HEAD']);head[42:44]=(32).to_bytes(2,'big');expected[b'HEAD']=bytes(head)
        appended=actual[b'SAMP'][len(base[b'SAMP']):];wav=(out/'host.wav').read_bytes();frames=(len(wav)-44)//6
        header=bytearray(64);name=b'BOUNCE ROWS 01-02';header[:len(name)]=name;header[32:36]=(48000).to_bytes(4,'big');header[36:40]=frames.to_bytes(4,'big');header[40:43]=bytes([24,2,64])
        pcm=b''.join(wav[i:i+3][::-1] for i in range(44,len(wav),3));record=bytes(header)+pcm;record+=bytes(-len(record)%4)
        assert appended==record;expected[b'SAMP']=base[b'SAMP']+record;assert actual==expected
        key(0x28,True);key(0x0c);screenshot=out/(run.name+'.png');time.sleep(.5);emu.command('SCREENSHOT',screenshot)
        wait(lambda:screenshot.exists() and screenshot.read_bytes().endswith(b'\0\0\0\0IEND\xaeB`\x82'));screenshot.rename(out/'bounced-sample.png')
        key(0x45);key(0x45);current='reopened.log';frame('status=READY -');key(0x21,True);frame('dirty=0 status=PROJECT SAVED')
        assert (run/'reopened.ptg').read_bytes()==(run/'saved.ptg').read_bytes()
        key(0x45);wait(lambda:(run/'done').exists());assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened','wav-copy','stem-copy'])
        assert (run/'excerpt.wav').read_bytes()==(out/'host.wav').read_bytes()==(run/'stems/track-01.wav').read_bytes()
        listing=(run/'ram-list.log').read_text();assert '.ptstems-' not in listing and '.pttmp-' not in listing
        (out/'ram-list.log').write_text(listing);shutil.copyfile(run/'excerpt.wav',out/'native.wav');shutil.copytree(run/'stems',out/'stems')
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PT24GConvert']},'fixture_sha256':digest(fixture),
            'true24_stereo_pcm_matches_host':True,'frames':frames,'slot_append_only_project_changes':True,
            'mix_cancellation_and_redo_preservation':True,'exact_reopen_identity':True,'stopped_audio':audio_off(),
            'logs':{n:(run/n).read_text() for n in ['editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        for n in ['baseline.ptg','saved.ptg','reopened.ptg']:shutil.copyfile(run/n,out/n)
        (out/'native-range-ui.json').write_text(json.dumps(report,indent=2)+'\n')
        print('PASS: selected-row WAV/stem/bounce parity, atomic slot, cancellation, redo and exact PTG persistence; '+str(out),flush=True)
    finally:
        if (run/"editor.log").exists():shutil.copyfile(run/"editor.log",out/"editor-capture.log")
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Bounce test released:',run,flush=True)
if __name__=='__main__':main()
