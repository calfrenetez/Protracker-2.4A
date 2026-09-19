#!/usr/bin/env python3
"""Reserve Amiberry first. Verify channel controls via native Intuition input."""
import json
from pathlib import Path
import shutil
import subprocess
import time
import zlib
from build_diagnostic import ROOT, digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket


def main():
    if matching_socket() or Path('/tmp/amiberry.sock').exists():
        raise SystemExit('Emulator already owned; reserve an exclusive window')
    env=json.loads((ROOT/'local/environment.json').read_text())
    share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('channels'+str(time.time_ns()));run.mkdir()
    out=ROOT/'build/dev/channel-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTPatternTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    source=(ROOT/'tests/fixtures/project-v1/mixed.ptg').read_bytes();(run/'input.ptg').write_bytes(source)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'classic.mod')
    process=emu=None;start=time.monotonic();current='editor.log';audio_checks=[]
    def log():return (run/current).read_text() if (run/current).exists() else ''
    def wait(condition,seconds=45):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            if condition():return
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            time.sleep(.1)
        raise RuntimeError('Channel test deadline: '+str(run)+' '+current)
    def frame(text):wait(lambda:any('EDITOR FRAME' in line and text in line for line in log().splitlines()))
    def key(raw,control=False,shift=False):
        before=len(log())
        if control:emu.command('SEND_KEY',0x63,1)
        if shift:emu.command('SEND_KEY',0x60,1)
        try:emu.tap(raw)
        finally:
            if shift:emu.command('SEND_KEY',0x60,0)
            if control:emu.command('SEND_KEY',0x63,0)
        wait(lambda:'EDITOR FRAME' in log()[before:] or 'EDITOR EXIT clean' in log()[before:])
    def capture(name):emu.command('SCREENSHOT',out/name)
    def volumes(values):
        before=len(log());target='volume='+','.join(map(str,values))
        wait(lambda:any('EDITOR REPLAY active=1' in line and target in line and 'period=428,339,285,214' in line for line in log()[before:].splitlines()))
        audio_checks.append(target)
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTPatternTest >pattern.log','Echo $RC >pattern.rc',
            'PT24GEdit input.ptg saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc',
            'PT24GEdit classic.mod classic.ptg >classic.log','Echo $RC >classic.rc',
            'PT24GEdit classic.ptg >classic-reopened.log','Echo $RC >classic-reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:
            process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert (run/'pattern.rc').read_text().strip()=='0'
        key(0x50);key(0x13,True);frame('panel=4')
        key(0x20);key(0x37);key(0x16);key(0x21);frame('revision=4 dirty=1 status=CHANNEL UPDATED')
        capture('01-midi-muted-solo-channel.png')
        key(0x51);key(0x19);frame('revision=5 dirty=1 status=CHANNEL UPDATED')
        key(0x42);key(0x19);frame('revision=5 dirty=1 status=PAULA LIMIT')
        capture('02-fifth-paula-refused.png')
        key(0x31,True);frame('revision=4 dirty=1 status=UNDO')
        key(0x20);key(0x31,True,True);frame('revision=5 dirty=1 status=REDO')
        key(0x50);key(0x21,True);frame('revision=5 dirty=0 status=PROJECT SAVED')
        saved=(run/'saved.ptg').read_bytes();expected=bytearray(source);pos=32
        for _ in range(int.from_bytes(source[24:28],'big')):
            tag=source[pos:pos+4];n=int.from_bytes(source[pos+8:pos+12],'big');body=pos+12
            if tag==b'HEAD':expected[body+33]=0
            if tag==b'CHAN':
                expected[body]=4;expected[body+2]=1;expected[body+3]=1
                expected[body+4*22]=1
            pos=body+n+(-n%4)
        expected[20:24]=bytes(4);expected[20:24]=zlib.crc32(expected).to_bytes(4,'big')
        assert saved==expected,'Saved channel bytes or unrelated data differ'
        key(0x45);key(0x45)
        current='reopened.log';frame('status=READY -');key(0x13,True);capture('03-reopened-channel-settings.png')
        key(0x21,True);frame('status=PROJECT SAVED');assert (run/'reopened.ptg').read_bytes()==saved
        key(0x45);key(0x45)
        current='classic.log';frame('status=READY -');key(0x13,True);key(0x57);volumes([24]*4)
        key(0x16);volumes([0,24,24,24]);capture('04-live-mute.png')
        key(0x42);key(0x21);volumes([0,24,0,0]);capture('05-live-solo.png')
        key(0x31,True);volumes([0,24,24,24])
        key(0x31,True);volumes([24]*4)
        key(0x31,True,True);volumes([0,24,24,24])
        key(0x37,True,True);frame('MOD EXPORT REFUSED')
        key(0x21,True);frame('status=PROJECT SAVED')
        key(0x59);audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        key(0x45);key(0x45)
        current='classic-reopened.log';frame('status=READY -');key(0x57);volumes([0,24,24,24])
        capture('06-saved-mute-on-restart.png')
        key(0x13,True);key(0x37);frame('STOPPED: EDIT REQUIRES ENHANCED REPLAY BACKEND')
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        key(0x31,True);key(0x45);key(0x45)
        wait(lambda:(run/'done').exists())
        for name in ['editor','reopened','classic','classic-reopened']:assert (run/(name+'.rc')).read_text().strip()=='0'
        assert (run/'input.ptg').read_bytes()==source
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{name:digest(run/name) for name in ['PT24GEdit','PTPatternTest']},
            'exact_channel_bytes_crc_and_unrelated_data_preserved':True,'reopened_byte_identity':True,
            'fifth_paula_refusal_preserves_history':True,'no_op_preserves_redo':True,
            'live_volume_checks':audio_checks,'strict_mod_export_refused_muted_project':True,
            'saved_mute_applied_on_restarted_playback':True,'unsupported_route_change_stops_dma':True,'stopped_audio':audio,'normal_exits':4,
            'logs':{name:(run/name).read_text() for name in ['pattern.log','editor.log','reopened.log','classic.log','classic-reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-channels.json').write_text(json.dumps(report,indent=2)+'\n')
        for name in ['saved.ptg','classic.ptg']:shutil.copyfile(run/name,out/name)
        print('PASS: native channel routes/mute/solo, undo/redo, Paula limit, exact save/reopen and live/restarted Paula mute')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Channel test released:',run)

if __name__=='__main__':main()
