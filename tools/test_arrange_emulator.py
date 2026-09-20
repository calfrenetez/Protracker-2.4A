#!/usr/bin/env python3
"""Native position insert/remove/reorder, shared undo and replay restart. Reserve Amiberry first."""
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
    run=share/('arrange'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/arrange-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTSongTest','PTPaulaTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'input.mod')
    baseline=(run/'input.mod').read_bytes()
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
    def capture(name):emu.command('SCREENSHOT',out/name)
    def audio_off():
        state=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in state.split('\t') for i in range(4));return state
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTSongTest native >pattern.log','Echo $RC >pattern.rc',
            'PTPaulaTest input.mod >paula.log','Echo $RC >paula.rc',
            'PT24GEdit input.mod saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['pattern','paula'])
        key(0x57);frame('PLAYING SONG - PAULA CIA');key(0x19,True)
        key(0x36);frame('revision=1 dirty=1 status=STOPPED: SONG POSITIONS CHANGED');audio_off()
        key(0x20);frame('revision=2 dirty=1 status=SONG UPDATED')
        key(0x4f);frame('revision=3 dirty=1 status=SONG UPDATED')
        for _ in range(3):key(0x31,True)
        frame('revision=0 dirty=0 status=UNDO')
        for _ in range(3):key(0x31,True,True)
        frame('revision=3 dirty=1 status=REDO')
        key(0x4d);key(0x45);key(0x40);key(0x31);frame('revision=4 dirty=1')
        key(0x19,True);key(0x31,True);key(0x31,True)
        frame('revision=2 dirty=1 status=UNDO')
        key(0x31,True,True);key(0x31,True,True);frame('revision=4 dirty=1 status=REDO')
        key(0x37);key(0x17);frame('revision=5 dirty=1 status=POSITIONS UPDATED')
        key(0x4c,shift=True);frame('revision=6 dirty=1 status=POSITIONS UPDATED')
        key(0x4d);key(0x22);frame('revision=7 dirty=1 status=POSITIONS UPDATED')
        key(0x4d,shift=True);frame('revision=8 dirty=1 status=POSITIONS UPDATED')
        for _ in range(4):key(0x31,True)
        frame('revision=4 dirty=1 status=UNDO')
        for _ in range(4):key(0x31,True,True)
        frame('revision=8 dirty=1 status=REDO')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED')
        saved=(run/'saved.ptg').read_bytes();check=bytearray(saved);check[20:24]=bytes(4);assert zlib.crc32(check)==int.from_bytes(saved[20:24],'big')
        expected=bytearray(baseline[:2108]+bytes(1024)+baseline[2108:])
        expected[950]=3;expected[952:955]=bytes([1,0,1]);expected[2108:2112]=bytes([1,172,16,0])
        request('mod');filename('exact.mod');key(0x44);frame('MOD EXPORTED AND VERIFIED')
        assert (run/'exact.mod').read_bytes()==expected,'Only the intended orders, appended pattern and note should differ'
        capture('01-position-arrangement.png')
        offset=key(0x57);wait(lambda:any('EDITOR REPLAY active=1' in row and 'order=2 pattern=1' in row and 'period=428,' in row for row in log()[offset:].splitlines()))
        key(0x4c,shift=True);frame('revision=9 dirty=1 status=STOPPED: SONG POSITIONS CHANGED');audio_off()
        key(0x31,True);frame('revision=8 dirty=0 status=UNDO');key(0x4d)
        offset=key(0x57);wait(lambda:any('EDITOR REPLAY active=1' in row and 'order=2 pattern=1' in row for row in log()[offset:].splitlines()))
        key(0x59);audio_off();key(0x45);key(0x45);current='reopened.log';frame('status=READY -')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');assert (run/'reopened.ptg').read_bytes()==saved
        key(0x19,True);key(0x37);capture('02-reopened-arrangement.png');key(0x45);key(0x45);wait(lambda:(run/'done').exists())
        assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened'])
        audio=audio_off()
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTSongTest','PTPaulaTest']},
            'native_growth_and_history_tests':True,'order_changes_stop_stale_playback':True,'restart_uses_new_pattern':True,
            'mixed_song_and_note_undo':True,'insert_remove_move_and_undo':True,'same_count_reorder_stops_audio':True,'exact_mod_orders_pattern_and_note_change':True,
            'exact_project_crc_and_reopen_identity':True,'normal_exits':2,'stopped_audio':audio,
            'logs':{n:(run/n).read_text() for n in ['pattern.log','paula.log','editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-arrange.json').write_text(json.dumps(report,indent=2)+'\n')
        for n in ['saved.ptg','reopened.ptg','exact.mod']:shutil.copyfile(run/n,out/n)
        print('PASS: native position arrangement, shared undo, replay restart, exact MOD export and PTG reopen')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Position arrangement test released:',run)
if __name__=='__main__':main()
