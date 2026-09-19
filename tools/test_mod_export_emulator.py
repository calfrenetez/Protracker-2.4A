#!/usr/bin/env python3
"""Native lossless MOD export/refusal. Requires exclusive Amiberry ownership."""
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
    run=share/('modexport'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/mod-export-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTModProjectTest','PTPaulaTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    baseline=(ROOT/'evidence/baseline/mod.baseline').read_bytes();(run/'a.mod').write_bytes(baseline)
    mixed=(ROOT/'tests/fixtures/project-v1/mixed.ptg').read_bytes();(run/'mixed.ptg').write_bytes(mixed)
    process=emu=None;start=time.monotonic();current='editor.log'
    def log():return (run/current).read_text() if (run/current).exists() else ''
    def wait(check,seconds=45):
        until=time.monotonic()+seconds
        while time.monotonic()<until:
            if check():return
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            time.sleep(.1)
        raise RuntimeError('MOD export deadline: '+str(run)+' '+current)
    def frame(text,offset=0):wait(lambda:any('EDITOR FRAME' in line and text in line for line in log()[offset:].splitlines()))
    def chord(raw,shift=False):
        emu.command('SEND_KEY',0x63,1)
        if shift:emu.command('SEND_KEY',0x60,1)
        try:emu.tap(raw)
        finally:
            if shift:emu.command('SEND_KEY',0x60,0)
            emu.command('SEND_KEY',0x63,0)
    def export():
        offset=len(log());chord(0x37,True);wait(lambda:'EDITOR REQUEST mod' in log()[offset:]);time.sleep(.8)
    def filename(name):
        keys=dict(zip('abcdefghijklmnopqrstuvwxyz',[0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31]));keys['.']=0x39
        emu.command('SEND_KEY',0x60,1)
        try:emu.tap(0x4f);emu.tap(0x46)
        finally:emu.command('SEND_KEY',0x60,0)
        for ch in name:emu.tap(keys[ch])
    def capture(name):emu.command('SCREENSHOT',out/name)
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTModProjectTest a.mod >mod-core.log','Echo $RC >mod-core.rc',
            'PTPaulaTest a.mod >paula.log','Echo $RC >paula.rc',
            'PT24GEdit a.mod classic.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit edited.mod >reopened.log','Echo $RC >reopened.rc',
            'PT24GEdit mixed.ptg rich.ptg >mixed.log','Echo $RC >mixed.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        wait(lambda:'status=READY -' in log(),90)
        for name in ['mod-core','paula']:assert (run/(name+'.rc')).read_text().strip()=='0',(run/(name+'.log')).read_text()
        export();capture('01-native-save-mod-requester.png');offset=len(log());emu.tap(0x45);frame('dirty=0 status=MOD EXPORT CANCELLED',offset)
        assert not (run/'new-module.mod').exists()
        export();offset=len(log());emu.tap(0x44);frame('dirty=0 status=MOD EXPORTED AND VERIFIED',offset)
        assert (run/'new-module.mod').read_bytes()==baseline
        emu.tap(0x40);emu.tap(0x32);frame('revision=1 dirty=1');export();offset=len(log());emu.tap(0x44)
        frame('revision=1 dirty=1 status=MOD EXPORT REFUSED: DESTINATION EXISTS',offset)
        assert (run/'new-module.mod').read_bytes()==baseline
        export();filename('edited.mod');offset=len(log());emu.tap(0x44);frame('revision=1 dirty=1 status=MOD EXPORTED AND VERIFIED',offset)
        expected=bytearray(baseline);expected[1084]=(expected[1084]&0xf0)|(381>>8);expected[1085]=381&255
        assert (run/'edited.mod').read_bytes()==expected
        capture('02-edited-mod-exported-project-still-dirty.png')
        chord(0x31);frame('revision=0 dirty=0 status=UNDO');chord(0x31,True);frame('revision=1 dirty=1 status=REDO')
        chord(0x21);frame('revision=1 dirty=0 status=PROJECT SAVED');emu.tap(0x45)
        current='reopened.log';frame('status=READY -');emu.tap(0x57);wait(lambda:'period=381,339,285,214' in log())
        capture('03-exported-mod-reopened-and-playing.png');emu.tap(0x59);frame('status=STOPPED - AUDIO RELEASED')
        export();filename('copy.mod');offset=len(log());emu.tap(0x44);frame('status=MOD EXPORTED AND VERIFIED',offset)
        assert (run/'copy.mod').read_bytes()==expected;emu.tap(0x45)
        current='mixed.log';frame('status=READY -');chord(0x37,True);frame('dirty=0 status=MOD EXPORT REFUSED: EXTERNAL MIDI')
        assert 'EDITOR REQUEST mod' not in log()
        emu.tap(0x50);emu.tap(0x40);emu.tap(0x31);frame('revision=1 dirty=1');offset=len(log());chord(0x37,True)
        frame('revision=1 dirty=1 status=MOD EXPORT REFUSED: EXTERNAL MIDI',offset);capture('04-enhanced-export-refused-with-edits-preserved.png')
        chord(0x21);frame('revision=1 dirty=0 status=PROJECT SAVED')
        rich=bytearray(mixed);pos=32
        for _ in range(int.from_bytes(mixed[24:28],'big')):
            tag=mixed[pos:pos+4];n=int.from_bytes(mixed[pos+8:pos+12],'big');body=pos+12
            if tag==b'HEAD':rich[body+33]=0
            if tag==b'PATT':rich[body:body+12]=bytes([1,1,1,172,0,0,0,0,0,0,0,0])
            pos=body+n+(-n%4)
        rich[20:24]=bytes(4);rich[20:24]=zlib.crc32(rich).to_bytes(4,'big')
        assert (run/'rich.ptg').read_bytes()==rich
        assert (run/'a.mod').read_bytes()==baseline and (run/'mixed.ptg').read_bytes()==mixed
        emu.tap(0x45);wait(lambda:(run/'done').exists())
        for name in ['editor','reopened','mixed']:assert (run/(name+'.rc')).read_text().strip()=='0'
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{name:digest(run/name) for name in ['PT24GEdit','PTModProjectTest','PTPaulaTest']},
            'classic_unedited_byte_identity':True,'edited_mod_exact_period_381':True,'cancel_and_existing_destination_preserved':True,
            'export_preserves_project_dirty_and_undo':True,'export_reopen_play_and_reexport_identity':True,
            'enhanced_refusal_before_dialog_with_exact_project_save':True,'normal_exits':3,'stopped_audio':audio,
            'logs':{name:(run/name).read_text() for name in ['editor.log','reopened.log','mixed.log','mod-core.log','paula.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL']}}
        (out/'native-mod-export.json').write_text(json.dumps(report,indent=2)+'\n')
        for name in ['edited.mod','rich.ptg']:shutil.copyfile(run/name,out/name)
        print('PASS: native genuine MOD export, exact edit/reopen/play/reexport, safe refusal/cancel, dirty/history preservation and enhanced-project retention')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('MOD export test released:',run)

if __name__=='__main__':main()
