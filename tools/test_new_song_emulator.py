#!/usr/bin/env python3
"""Native staged New Song workflow; reserve the private emulator first."""
import json
from pathlib import Path
import shutil
import subprocess
import time
import zlib
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket


def inspect(data,channels,selected,note):
    crc=bytearray(data);crc[20:24]=bytes(4);assert zlib.crc32(crc)==int.from_bytes(data[20:24],'big')
    parts={};pos=32
    for _ in range(int.from_bytes(data[24:28],'big')):
        n=int.from_bytes(data[pos+8:pos+12],'big');parts[data[pos:pos+4]]=data[pos+12:pos+12+n];pos+=12+n+(-n%4)
    assert set(parts)=={b'HEAD',b'CHAN',b'ORDR',b'PATT',b'SAMP',b'MIDI'}
    head=parts[b'HEAD'];assert head[:32]==bytes(32) and head[32:38]==bytes([channels,selected,6,0,0,125])
    assert head[38:44]==bytes([0,1,0,1,0,31]) and parts[b'ORDR']==bytes(2)
    expected=bytearray(channels*64*12)
    if note:expected[15*12:16*12]=bytes([1,1,1,172,0,0,0,0,0,0,0,0])
    assert parts[b'PATT']==expected
    assert len(parts[b'CHAN'])==channels*22
    for c in range(channels):assert parts[b'CHAN'][c*22]==(1 if c<4 else 2)
    sample=bytearray(64);sample[32:36]=(8287).to_bytes(4,'big');sample[40:42]=bytes([8,1])
    assert parts[b'SAMP']==sample*31


def main():
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('newsong'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/new-song-evidence';out.mkdir(exist_ok=True)
    for n in ['PT24GEdit','PTDocumentTest']:shutil.copyfile(ROOT/'build/dev'/n,run/n)
    shutil.copyfile(ROOT/'tests/fixtures/project-v1/mixed.ptg',run/'mixed.ptg');shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'a.mod')
    process=emu=None;start=time.monotonic();current='editor.log'
    def log():return (run/current).read_text() if (run/current).exists() else ''
    def wait(check,seconds=45):
        until=time.monotonic()+seconds
        while time.monotonic()<until:
            if check():return
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            time.sleep(.1)
        raise RuntimeError('New Song deadline: '+str(run)+' '+current)
    def frame(text,offset=0):wait(lambda:any('EDITOR FRAME' in line and text in line for line in log()[offset:].splitlines()))
    def key(raw,ctrl=False,shift=False):
        offset=len(log())
        if ctrl:emu.command('SEND_KEY',0x63,1)
        if shift:emu.command('SEND_KEY',0x60,1)
        try:emu.tap(raw)
        finally:
            if shift:emu.command('SEND_KEY',0x60,0)
            if ctrl:emu.command('SEND_KEY',0x63,0)
        wait(lambda:'EDITOR FRAME' in log()[offset:] or 'EDITOR REQUEST' in log()[offset:] or 'EDITOR EXIT' in log()[offset:])
    def capture(name):emu.command('SCREENSHOT',out/name)
    def filename(name):
        keys=dict(zip('abcdefghijklmnopqrstuvwxyz',[0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31]));keys['.']=0x39
        emu.command('SEND_KEY',0x60,1)
        try:emu.tap(0x4f);emu.tap(0x46)
        finally:emu.command('SEND_KEY',0x60,0)
        for ch in name:emu.tap(keys[ch])
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTDocumentTest a.mod >document.log','Echo $RC >document.rc',
            'PT24GEdit a.mod sixteen.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit sixteen.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        wait(lambda:'status=READY -' in log(),90);assert (run/'document.rc').read_text().strip()=='0'
        key(0x57);wait(lambda:'EDITOR REPLAY active=1' in log());key(0x36,True)
        for _ in range(12):key(0x0c)
        capture('01-new-sixteen-channel-song.png');key(0x44);frame('status=NEW SONG READY')
        assert 'EDITOR NEW channels=16 patterns=1' in log()
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        key(0x53)
        for _ in range(3):key(0x42)
        key(0x40);key(0x31);frame('row=1 channel=15 revision=1 dirty=1')
        key(0x36,True);key(0x44);frame('UNSAVED EDITS: CREATE AGAIN');capture('02-create-discard-confirmation.png')
        key(0x0b);frame('NEW SONG CANCELLED - EDITS PRESERVED');key(0x45)
        assert log().count('EDITOR NEW channels=')==1
        key(0x31,True);frame('revision=0 dirty=0 status=UNDO');key(0x31,True,True);frame('revision=1 dirty=1 status=REDO')
        key(0x21,True);frame('revision=1 dirty=0 status=PROJECT SAVED')
        sixteen=(run/'sixteen.ptg').read_bytes();inspect(sixteen,16,15,True);capture('03-sixteen-channel-project-saved.png')
        key(0x31);frame('revision=2 dirty=1');key(0x36,True)
        for _ in range(15):key(0x0b)
        key(0x44);frame('revision=2 dirty=1 status=UNSAVED EDITS: CREATE AGAIN');offset=len(log());key(0x44)
        frame('row=0 channel=0 revision=0 dirty=0 status=NEW SONG READY',offset)
        assert 'EDITOR NEW channels=1 patterns=1' in log();key(0x34,True);frame('CLIPBOARD EMPTY')
        key(0x21,True,True);time.sleep(.8);filename('single.ptg');offset=len(log());emu.tap(0x44);frame('status=PROJECT SAVED',offset)
        single=(run/'single.ptg').read_bytes();inspect(single,1,0,False);capture('04-single-channel-project.png')
        key(0x45);current='reopened.log';frame('status=READY -');key(0x21,True);frame('status=PROJECT SAVED')
        assert (run/'reopened.ptg').read_bytes()==sixteen;capture('05-sixteen-channel-project-reopened.png');key(0x45)
        wait(lambda:(run/'done').exists())
        for n in ['editor','reopened']:assert (run/(n+'.rc')).read_text().strip()=='0'
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTDocumentTest']},
            'native_all_counts_and_allocation_failure_tests':(run/'document.log').read_text(),'old_audio_stopped_after_create':audio,
            'dirty_confirmation_cancel_preserves_undo':True,'sixteen_channel_note_exact_fields_and_crc':True,'one_channel_blank_exact_fields_and_crc':True,
            'reopened_byte_identity':True,'normal_exits':2,'logs':{n:(run/n).read_text() for n in ['editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL']}}
        (out/'native-new-song.json').write_text(json.dumps(report,indent=2)+'\n')
        for n in ['sixteen.ptg','single.ptg']:shutil.copyfile(run/n,out/n)
        print('PASS: native staged 1..16 channel creation, allocation failures, old playback stopped, dirty cancellation/confirmation, exact saves/reopen')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('New Song test released:',run)

if __name__=='__main__':main()
