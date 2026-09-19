#!/usr/bin/env python3
"""Native replay ownership and real Enhanced UI test. Reserve Amiberry first."""
import json
from pathlib import Path
import shutil
import subprocess
import time
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket


def main():
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('playback'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/playback-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTPaulaTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'input.mod')
    process=emu=None;start=time.monotonic()
    def contents(name):return (run/name).read_text() if (run/name).exists() else ''
    def wait(check,seconds=40):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            if check():return
            if process.poll() is not None:raise RuntimeError('Emulator exited: '+str(run))
            time.sleep(.1)
        raise RuntimeError('Playback test timed out: '+str(run))
    def frame(name,fragment):wait(lambda:any('EDITOR FRAME' in s and fragment in s for s in contents(name).splitlines()))
    def replay(name,fragment):wait(lambda:any('EDITOR REPLAY active=1' in s and fragment in s for s in contents(name).splitlines()))
    def capture(name):emu.command('SCREENSHOT',out/name)
    def save():
        emu.command('SEND_KEY',0x63,1)
        try:emu.tap(0x21)
        finally:emu.command('SEND_KEY',0x63,0)
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTPaulaTest input.mod >paula.log','Echo $RC >paula.rc',
            'PT24GEdit input.mod saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as log:
            process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()),45);matches=matching_socket()
        if len(matches)!=1:raise RuntimeError('Ambiguous emulator')
        emu=Emulator(matches[0]);wait(lambda:(run/'paula.rc').exists(),60)
        (out/'native-paula.log').write_text(contents('paula.log'))
        assert contents('paula.rc').strip()=='0' and 'PAULA PASS' in contents('paula.log'),contents('paula.log')
        frame('editor.log','status=READY -');emu.tap(0x40);emu.tap(0x57)
        replay('editor.log','period=428,339,285,214');capture('01-playing-classic.png')
        audio_play=emu.command('GET_AUDIO_STATE')
        assert all('ch%d_dma=1'%i in audio_play.split('\t') for i in range(4)),audio_play
        # The edit cursor remains independent of replay position.
        emu.tap(0x32);frame('editor.log','revision=1 dirty=1 status=PATTERN EDITED')
        emu.tap(0x59);frame('editor.log','status=STOPPED - AUDIO RELEASED')
        audio_stop=emu.command('GET_AUDIO_STATE')
        assert all('ch%d_dma=0'%i in audio_stop.split('\t') for i in range(4)),audio_stop
        emu.tap(0x58);replay('editor.log','period=381,339,285,214');capture('02-playing-edited-pattern.png')
        save();frame('editor.log','revision=1 dirty=0 status=PROJECT SAVED');emu.tap(0x59);emu.tap(0x45)
        wait(lambda:(run/'editor.rc').exists());assert contents('editor.rc').strip()=='0'
        frame('reopened.log','status=READY -');emu.tap(0x57);replay('reopened.log','period=381,339,285,214')
        capture('03-reopened-edited-playback.png');save();frame('reopened.log','status=PROJECT SAVED');emu.tap(0x59);emu.tap(0x45)
        wait(lambda:(run/'done').exists());assert contents('reopened.rc').strip()=='0'
        assert (run/'saved.ptg').read_bytes()==(run/'reopened.ptg').read_bytes()
        assert (run/'input.mod').read_bytes()==(ROOT/'evidence/baseline/mod.baseline').read_bytes()
        capture('04-normal-exit.png')
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTPaulaTest']},
            'native_ownership_tests':contents('paula.log'),'editor':contents('editor.log'),'reopened':contents('reopened.log'),
            'playback_audio_state':audio_play,'stopped_audio_state':audio_stop,
            'reopened_byte_identity':True,'input_mod_preserved':True,
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-playback.json').write_text(json.dumps(report,indent=2)+'\n');shutil.copyfile(run/'saved.ptg',out/'edited.ptg')
        print('PASS: native ownership/fallback/live edit/audition and Enhanced UI play/edit/save/reopen replay')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:
                try:process.wait(timeout=5)
                except subprocess.TimeoutExpired:process.terminate();process.wait(timeout=5)
        print('Private playback test exited:',run)
if __name__=='__main__':main()
