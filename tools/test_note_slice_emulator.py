#!/usr/bin/env python3
"""Native pattern slice assignment and shared sampler history. Reserve private Amiberry first."""
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
    run=share/('noteslice'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/note-slice-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTNoteSliceTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    process=emu=None;start=time.monotonic();current='editor.log'
    def log():return (run/current).read_text() if (run/current).exists() else ''
    def wait(check,seconds=60):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            if check():return
            if process.poll() is not None:raise RuntimeError('Emulator exited: '+str(run))
            time.sleep(.1)
        raise RuntimeError('Details deadline '+str(run)+' '+current+' '+log()[-500:])
    def frame(text,after=0):wait(lambda:any('EDITOR FRAME' in s and text in s for s in log()[after:].splitlines()))
    def key(raw,control=False,shift=False):
        offset=len(log())
        if control:emu.command('SEND_KEY',0x63,1)
        if shift:emu.command('SEND_KEY',0x60,1)
        try:emu.tap(raw)
        finally:
            if shift:emu.command('SEND_KEY',0x60,0)
            if control:emu.command('SEND_KEY',0x63,0)
        wait(lambda:'EDITOR FRAME' in log()[offset:] or 'EDITOR EXIT clean' in log()[offset:])
        return offset
    def capture(name):emu.command('SCREENSHOT',out/name)
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTNoteSliceTest input.ptg >notes.log','Echo $RC >notes.rc',
            'PT24GEdit input.ptg saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert (run/'notes.rc').read_text().strip()=='0';source=(run/'input.ptg').read_bytes()
        key(0x17,True);key(0x0c);frame('revision=1 dirty=1 status=NOTE SLICE SAVED')
        key(0x21);key(3);key(0x44);frame('revision=2 dirty=1 status=NOTE SLICE SAVED')
        key(0x31,True);key(0x21);key(4);key(0x44);frame('revision=1 dirty=1 status=SLICE REFUSED')
        key(0x42);key(0x45);key(0x31,True,True);frame('revision=2 dirty=1 status=REDO')
        key(0x0b);frame('revision=3 dirty=1 status=NOTE SLICE SAVED');capture('01-pattern-slice-two.png')
        key(0x28);key(0x42);key(0x42);key(0x33);key(0x19)
        frame('revision=3 dirty=1 status=SLICE CHANGE WOULD RETARGET');capture('02-referenced-marker-refused.png');key(0x32)
        key(0x17,True);key(0x33);frame('revision=4 dirty=1 status=NOTE USES WHOLE SAMPLE')
        key(0x28);key(0x42);key(0x42);key(0x33);key(0x19);frame('revision=5 dirty=1 status=SLICE MARKERS APPLIED')
        key(0x31,True);key(0x31,True);frame('revision=3 dirty=1 status=UNDO')
        key(0x17,True);key(0x51);key(0x16);frame('revision=6 dirty=1 status=NOTE INSTRUMENT SET')
        key(0x0c);frame('revision=7 dirty=1 status=NOTE SLICE SAVED');capture('03-channel-five-slice-one.png')
        key(0x50);key(0x21,True);frame('revision=7 dirty=0 status=PROJECT SAVED')
        saved=(run/'saved.ptg').read_bytes();expected=bytearray(source);pos=32
        for _ in range(int.from_bytes(source[24:28],'big')):
            size=int.from_bytes(source[pos+8:pos+12],'big');body=pos+12
            if source[pos:pos+4]==b'PATT':
                expected[body+8:body+10]=(2).to_bytes(2,'big')
                expected[body+4*12+1]=1;expected[body+4*12+8:body+4*12+10]=(1).to_bytes(2,'big')
            pos=body+size+(-size%4)
        expected[20:24]=bytes(4);expected[20:24]=zlib.crc32(expected).to_bytes(4,'big')
        assert saved==expected,'Only selected note instrument/slice bytes should differ'
        key(0x17,True);key(0x12,True);key(0x45);current='reopened.log';frame('status=READY -')
        key(0x17,True);capture('04-reopened-note-slice.png');key(0x21,True);frame('status=PROJECT SAVED')
        assert (run/'reopened.ptg').read_bytes()==saved
        key(0x17,True);key(0x12,True);key(0x45);wait(lambda:(run/'done').exists())
        assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened'])
        assert (run/'input.ptg').read_bytes()==source
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTNoteSliceTest']},
            'native_controller_tests':True,'one_based_slice_mapping':True,'invalid_input_preserves_redo':True,
            'referenced_marker_removal_refused':True,'shared_note_and_sampler_undo_order':True,
            'explicit_sample_attachment_on_channel5':True,'exact_note_bytes_crc_and_unrelated_data_preserved':True,
            'reopened_byte_identity':True,'normal_exits':2,'stopped_audio':audio,
            'logs':{n:(run/n).read_text() for n in ['notes.log','editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-note-slice.json').write_text(json.dumps(report,indent=2)+'\n');shutil.copyfile(run/'saved.ptg',out/'saved.ptg')
        print('PASS: native pattern slice assignment, sample attachment, shared marker/note undo, exact bytes and reopen')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Note slice test released:',run)
if __name__=='__main__':main()
