#!/usr/bin/env python3
"""Native added sample slots, selected instrument import, undo and PTG persistence. Reserve Amiberry first."""
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
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('slots'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/slots-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTSlotsTest','PT24GConvert']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'input.mod')
    process=emu=None;start=time.monotonic();current='editor.log'
    def log():return (run/current).read_text() if (run/current).exists() else ''
    def wait(check,seconds=180):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            if check():return
            if process.poll() is not None:raise RuntimeError('Emulator exited: '+str(run))
            time.sleep(.1)
        raise RuntimeError('Sampler deadline '+str(run)+' '+current+' '+log()[-400:])
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
    def request(kind):
        offset=key(0x37 if kind=='mod' else 0x28,control=kind=='mod',shift=kind=='mod',ack=False)
        wait(lambda:('EDITOR REQUEST '+kind) in log()[offset:]);time.sleep(.8)
    def filename(text):
        keys=dict(zip('abcdefghijklmnopqrstuvwxyz',[0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31]));keys['.']=0x39
        emu.command('SEND_KEY',0x60,1)
        try:emu.tap(0x4f);emu.tap(0x46)
        finally:emu.command('SEND_KEY',0x60,0)
        for ch in text:emu.tap(keys[ch])
    def capture(name):
        # SCREENSHOT acknowledgement can precede the queued display capture.
        # Wait for a fresh complete PNG before later input can close the screen.
        path=out/(run.name+'-'+name);time.sleep(.5);emu.command('SCREENSHOT',path)
        wait(lambda:path.exists() and path.read_bytes().endswith(b'\0\0\0\0IEND\xaeB`\x82'))
        path.replace(out/name)
    def audio_off():
        state=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in state.split('\t') for i in range(4));return state
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTSlotsTest >slots.log','Echo $RC >slots.rc',
            'PT24GConvert project input.mod baseline.ptg >convert.log','Echo $RC >convert.rc',
            'PT24GEdit input.mod saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['slots','convert'])
        def chunks(data):
            check=bytearray(data);check[20:24]=bytes(4);assert zlib.crc32(check)==int.from_bytes(data[20:24],'big')
            result={};pos=32
            for _ in range(int.from_bytes(data[24:28],'big')):
                size=int.from_bytes(data[pos+8:pos+12],'big');result[data[pos:pos+4]]=data[pos+12:pos+12+size];pos+=12+size+(-size%4)
            assert pos==len(data);return result
        key(0x57);frame('PLAYING SONG - PAULA CIA');key(0x28,True)
        key(0x0c,True,True);frame('revision=1 dirty=1 status=SAMPLE SLOT ADDED');audio_off()
        request('sample');filename('input.mod');key(0x44);frame('MOD SOURCE READY')
        key(0x17);frame('revision=2 dirty=1 status=SOURCE INSTRUMENT IMPORTED');key(0x45)
        offset=key(0x57);frame('SAMPLE AUDITION - PAULA')
        wait(lambda:any('EDITOR REPLAY active=1' in row and 'period=428,' in row for row in log()[offset:].splitlines()))
        key(0x31,True);frame('revision=1 dirty=1 status=UNDO');audio_off()
        key(0x31,True);frame('revision=0 dirty=0 status=UNDO')
        key(0x31,True,True);key(0x31,True,True);key(0x0c)
        key(0x45);key(0x40);key(0x31);frame('revision=3 dirty=1 status=PATTERN EDITED')
        for _ in range(3):key(0x31,True)
        frame('revision=0 dirty=0 status=UNDO')
        for _ in range(3):key(0x31,True,True)
        frame('revision=3 dirty=1 status=REDO')
        key(0x37,True,True);frame('MOD EXPORT REFUSED')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED')
        saved=(run/'saved.ptg').read_bytes();base=chunks((run/'baseline.ptg').read_bytes());actual=chunks(saved);expected=dict(base)
        info=bytearray(base[b'HEAD']);info[42:44]=(32).to_bytes(2,'big');expected[b'HEAD']=bytes(info)
        pattern=bytearray(base[b'PATT']);pattern[1]=32;expected[b'PATT']=bytes(pattern)
        sample=base[b'SAMP'];frames=int.from_bytes(sample[36:40],'big');markers=int.from_bytes(sample[46:48],'big')
        record=64+markers*4+frames*sample[41]*(sample[40]//8);record+=-record%4
        expected[b'SAMP']=sample+sample[:record]
        assert actual==expected,'Only slot count, appended duplicate sample and instrument reference should change'
        key(0x28,True);key(0x0c);capture('01-sample-slot-32.png');key(0x45);key(0x45)
        current='reopened.log';frame('status=READY -');key(0x21,True);frame('dirty=0 status=PROJECT SAVED')
        assert (run/'reopened.ptg').read_bytes()==saved
        key(0x17,True);key(0x28);capture('02-reopened-slot-32.png');key(0x45);key(0x45);wait(lambda:(run/'done').exists())
        assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened'])
        audio=audio_off()
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTSlotsTest','PT24GConvert']},
            'native_slot_ownership_tests':True,'slot_32_import_and_classic_audition':True,
            'mixed_slot_sample_and_note_undo':True,'strict_mod_refuses_extra_slot':True,
            'exact_project_chunk_changes_and_reopen_identity':True,'normal_exits':2,'stopped_audio':audio,
            'logs':{n:(run/n).read_text() for n in ['slots.log','convert.log','editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-slots.json').write_text(json.dumps(report,indent=2)+'\n')
        for n in ['saved.ptg','reopened.ptg','baseline.ptg']:shutil.copyfile(run/n,out/n)
        print('PASS: native sample slots, import/audition, shared undo, strict MOD refusal and exact PTG reopen')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Sample slots test released:',run)
if __name__=='__main__':main()
