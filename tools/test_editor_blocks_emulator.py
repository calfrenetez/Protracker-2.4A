#!/usr/bin/env python3
"""Reserve Amiberry first; exercise block commands through real Intuition keys."""
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
    run=share/('blocks'+str(time.time_ns()));run.mkdir()
    out=ROOT/'build/dev/block-evidence';out.mkdir(exist_ok=True)
    shutil.copyfile(ROOT/'build/dev/PT24GEdit',run/'PT24GEdit')
    source=(ROOT/'tests/fixtures/project-v1/mixed.ptg').read_bytes();(run/'input.ptg').write_bytes(source)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'classic.mod')
    process=emu=None;start=time.monotonic();current='editor.log'
    def log():return (run/current).read_text() if (run/current).exists() else ''
    def wait(condition,seconds=45):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            if condition():return
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            time.sleep(.1)
        raise RuntimeError('Block test deadline: '+str(run)+' '+current)
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
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PT24GEdit input.ptg saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc',
            'PT24GEdit classic.mod >classic.log','Echo $RC >classic.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:
            process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');key(0x50);key(0x12,True);frame('panel=1')
        key(0x35,True);key(0x4d);key(0x42);key(0x33,True)
        frame('COPIED 2 ROWS X 2 CHANNELS');capture('01-marked-block-and-edit-operations.png')
        key(0x35,True);key(0x51)
        for _ in range(3):key(0x4d)
        key(0x34,True);frame('revision=1 dirty=1 status=BLOCK PASTED')
        key(0x35,True);key(0x4d);key(0x42);key(0x0c,True)
        frame('revision=2 dirty=1 status=BLOCK TRANSPOSED');capture('02-transposed-copy.png')
        key(0x46,True);frame('revision=3 dirty=1 status=BLOCK CLEARED')
        key(0x31,True);frame('revision=2 dirty=1 status=UNDO')
        key(0x31,True);frame('revision=1 dirty=1 status=UNDO')
        key(0x31,True,True);frame('revision=2 dirty=1 status=REDO')
        key(0x35,True);key(0x53)
        for _ in range(3):key(0x42)
        key(0x34,True);frame('revision=2 dirty=1 status=PASTE WOULD CROSS PATTERN EDGE')
        capture('03-edge-refusal-preserves-edits.png')
        # Clone explicitly into existing pattern 1, then undo/redo the full block.
        key(0x20,True);key(0x33,True);frame('COPIED 64 ROWS X 16 CHANNELS')
        key(0x5b);key(0x50)
        for _ in range(5):key(0x4c)
        key(0x34,True);frame('revision=4 dirty=1 status=BLOCK PASTED')
        key(0x31,True);frame('revision=2 dirty=1 status=UNDO')
        key(0x31,True,True);frame('revision=4 dirty=1 status=REDO')
        key(0x21,True);frame('revision=4 dirty=0 status=PROJECT SAVED')
        saved=(run/'saved.ptg').read_bytes();expected=bytearray(source);pos=32
        for _ in range(int.from_bytes(source[24:28],'big')):
            tag=source[pos:pos+4];n=int.from_bytes(source[pos+8:pos+12],'big');body=pos+12
            if tag==b'HEAD':expected[body+33]=0
            if tag==b'PATT':
                for r in range(2):
                    for c in range(2):
                        src=body+(r*16+c)*12;dest=body+((4+r)*16+4+c)*12
                        expected[dest:dest+12]=source[src:src+12]
                        if expected[dest]==1:
                            assert int.from_bytes(expected[dest+2:dest+4],'big')==428
                            expected[dest+2:dest+4]=(404).to_bytes(2,'big')
                expected[body+1024*12:body+2048*12]=expected[body:body+1024*12]
            pos=body+n+(-n%4)
        expected[20:24]=bytes(4);expected[20:24]=zlib.crc32(expected).to_bytes(4,'big')
        assert saved==expected,'Block save differs from independently specified copied/transposed/cloned events'
        capture('04-pattern-cloned-and-saved.png');key(0x45)
        current='reopened.log';frame('status=READY -');key(0x34,True);frame('CLIPBOARD EMPTY')
        key(0x21,True);frame('status=PROJECT SAVED');assert (run/'reopened.ptg').read_bytes()==saved
        key(0x45);current='classic.log';frame('status=READY -')
        key(0x57);wait(lambda:'period=428,' in log());key(0x35,True);key(0x0c,True)
        frame('BLOCK TRANSPOSED');key(0x58);wait(lambda:'period=404,' in log())
        capture('05-block-edit-playing-C-sharp.png');key(0x59)
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        key(0x45);key(0x45);wait(lambda:(run/'done').exists())
        for name in ['editor','reopened','classic']:assert (run/(name+'.rc')).read_text().strip()=='0'
        logs={name:(run/name).read_text() for name in ['editor.log','reopened.log','classic.log']}
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'binary_sha256':digest(run/'PT24GEdit'),
            'saved_sha256':digest(run/'saved.ptg'),'exact_copy_transpose_clone_bytes_and_crc':True,
            'whole_block_undo_redo':True,'edge_refusal_preserves_revision':True,'clipboard_reset_on_open':True,
            'reopened_byte_identity':True,'classic_block_edit_played_period_404':True,'stopped_audio':audio,
            'normal_exits':3,'logs':logs,'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL']}}
        (out/'native-blocks.json').write_text(json.dumps(report,indent=2)+'\n');shutil.copyfile(run/'saved.ptg',out/'saved.ptg')
        print('PASS: native block copy/transpose/clear/undo/redo/edge refusal/clone, exact save/reopen, clipboard reset and Paula playback')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Block test released:',run)

if __name__=='__main__':main()
