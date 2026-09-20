#!/usr/bin/env python3
"""Native channel metadata entry/persistence. Reserve private Amiberry first."""
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
    run=share/('details'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/details-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTModProjectTest','PTPaulaTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    source=(ROOT/'tests/fixtures/project-v1/mixed.ptg').read_bytes();(run/'input.ptg').write_bytes(source)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'input.mod')
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
            'PTModProjectTest input.mod >mod.log','Echo $RC >mod.rc',
            'PTPaulaTest input.mod >paula.log','Echo $RC >paula.rc',
            'PT24GEdit input.ptg saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc',
            'PT24GEdit input.mod classic.ptg >classic.log','Echo $RC >classic.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['mod','paula'])
        key(0x50);key(0x13,True);key(0x22);key(0x19);key(8);key(0x0a);key(0x44)
        frame('revision=1 dirty=1 status=CHANNEL UPDATED')
        key(0x24);key(0x23);key(0x44);frame('revision=2 dirty=1 status=CHANNEL UPDATED')
        key(0x37);key(1);key(7);key(0x44);frame('revision=2 dirty=1 status=INVALID CHANNEL VALUE')
        key(0x42);key(0x41);key(6);key(0x44);frame('revision=3 dirty=1 status=CHANNEL UPDATED')
        key(0x36);key(0x35);key(0x20);key(0x21);key(0x21);key(0x44);frame('revision=4 dirty=1 status=CHANNEL UPDATED')
        capture('01-channel-details.png')
        key(0x31,True);frame('revision=3 dirty=1 status=UNDO')
        key(0x36);key(0x32);key(0x45);frame('revision=3 dirty=1 status=TRACK NAME CANCELLED')
        key(0x31,True,True);frame('revision=4 dirty=1 status=REDO')
        key(0x51);key(0x36);key(0x19);key(0x20);key(0x22);key(0x44);frame('revision=5 dirty=1 status=CHANNEL UPDATED')
        key(0x50);key(0x21,True);frame('revision=5 dirty=0 status=PROJECT SAVED')
        saved=(run/'saved.ptg').read_bytes();expected=bytearray(source);pos=32
        for _ in range(int.from_bytes(source[24:28],'big')):
            size=int.from_bytes(source[pos+8:pos+12],'big');body=pos+12
            if source[pos:pos+4]==b'HEAD':expected[body+33]=0
            if source[pos:pos+4]==b'CHAN':
                expected[body+1]=128;expected[body+4]=15;expected[body+5]=16
                expected[body+6:body+22]=b'BASS'+bytes(12)
                expected[body+4*22+6:body+5*22]=b'PAD'+bytes(13)
            pos=body+size+(-size%4)
        expected[20:24]=bytes(4);expected[20:24]=zlib.crc32(expected).to_bytes(4,'big')
        assert saved==expected,'Channel settings or unrelated song bytes differ'
        key(0x45);key(0x45);key(0x45);current='reopened.log';frame('status=READY -')
        key(0x13,True);key(0x22);capture('02-reopened-details.png');key(0x21,True);frame('status=PROJECT SAVED')
        assert (run/'reopened.ptg').read_bytes()==saved
        key(0x45);key(0x45);key(0x45);current='classic.log';frame('status=READY -')
        key(0x13,True);key(0x22);key(0x37);key(1);key(6);key(0x44)
        key(0x37,True,True);frame('MOD EXPORT REFUSED');capture('03-midi-metadata-export-refused.png')
        key(0x31,True);frame('revision=0 dirty=0 status=UNDO');key(0x45);key(0x45);key(0x45)
        wait(lambda:(run/'done').exists());assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened','classic'])
        assert (run/'input.ptg').read_bytes()==source
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTModProjectTest','PTPaulaTest']},
            'paula_replay_preserves_organisation_metadata':True,'hex_pan_and_group_decimal_midi':True,'invalid_entry_no_change':True,'modal_channel_selection_frozen':True,
            'cancel_preserves_redo':True,'names_across_channel_pages':True,'exact_channel_bytes_crc_and_unrelated_data_preserved':True,
            'reopened_byte_identity':True,'strict_mod_refuses_only_midi_assignment':True,'normal_exits':3,'stopped_audio':audio,
            'logs':{n:(run/n).read_text() for n in ['mod.log','paula.log','editor.log','reopened.log','classic.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-details.json').write_text(json.dumps(report,indent=2)+'\n');shutil.copyfile(run/'saved.ptg',out/'saved.ptg')
        print('PASS: native channel names/pan/groups/MIDI, bounds, modal isolation, undo/redo, exact save/reopen and strict MOD loss refusal')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Channel details test released:',run)
if __name__=='__main__':main()
