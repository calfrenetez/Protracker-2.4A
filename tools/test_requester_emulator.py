#!/usr/bin/env python3
"""Native ASL load/save/cancel/dirty tests. Requires reserved private emulator."""
import argparse
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
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--previous-view-binary",type=Path);args=parser.parse_args()
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('requester'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/requester-evidence';out.mkdir(exist_ok=True)
    shutil.copyfile(ROOT/'build/dev/PT24GEdit',run/'PT24GEdit');shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'a.mod');(run/'bad.mod').write_text('invalid module')
    shutil.copyfile(ROOT/'build/dev/PTViewBench',run/'PTViewBench')
    if args.previous_view_binary:shutil.copyfile(args.previous_view_binary,run/'PTViewBefore')
    process=emu=None;start=time.monotonic()
    def log():return (run/'editor.log').read_text() if (run/'editor.log').exists() else ''
    def wait(check,seconds=40):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            if check():return
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            time.sleep(.1)
        raise RuntimeError('Requester deadline: '+str(run))
    def frame(fragment,after=0):wait(lambda:any('EDITOR FRAME' in line and fragment in line for line in log()[after:].splitlines()))
    def chord(key,shift=False):
        emu.command('SEND_KEY',0x63,1)
        if shift:emu.command('SEND_KEY',0x60,1)
        try:emu.tap(key)
        finally:
            emu.command('SEND_KEY',0x63,0)
            if shift:emu.command('SEND_KEY',0x60,0)
    def request(kind,dirty=False,shift=False):
        if dirty:
            offset=len(log());chord(0x18);frame('status=UNSAVED EDITS: LOAD AGAIN',offset)
        offset=len(log());chord(0x18 if kind=='load' else 0x21,shift)
        wait(lambda:'EDITOR REQUEST '+kind in log()[offset:]);time.sleep(.8)
    def filename(text):
        keys=dict(zip('abcdefghijklmnopqrstuvwxyz',[0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31]));keys.update({'.':0x39,'-':0x0b})
        emu.command('SEND_KEY',0x60,1)
        try:emu.tap(0x4f);emu.tap(0x46)
        finally:emu.command('SEND_KEY',0x60,0)
        for ch in text:emu.tap(keys[ch])
    def capture(name):emu.command('SCREENSHOT',out/name)
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,'PTViewBench a.mod >view-after.log']+(['PTViewBefore a.mod >view-before.log'] if args.previous_view_binary else [])+['PT24GEdit >editor.log','Echo $RC >editor.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()),45);matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        wait(lambda:'EDITOR FRAME' in log() and 'status=READY -' in log(),90);capture('01-blank-song.png')
        request('load');capture('02-load-requester.png');filename('a.mod');offset=len(log());emu.tap(0x44);frame('status=PROJECT LOADED',offset)
        emu.tap(0x40);emu.tap(0x32);frame('revision=1 dirty=1');request('save');capture('03-save-requester.png')
        offset=len(log());emu.tap(0x44);frame('revision=1 dirty=0 status=PROJECT SAVED',offset)
        saved=(run/'new-project.ptg').read_bytes();crc=bytearray(saved);crc[20:24]=bytes(4);assert zlib.crc32(crc)==int.from_bytes(saved[20:24],'big')
        pos=32;found=False
        while pos<len(saved):
            n=int.from_bytes(saved[pos+8:pos+12],'big')
            if saved[pos:pos+4]==b'PATT':assert saved[pos+12:pos+16]==bytes([1,1,1,125]);found=True
            pos+=12+n+(-n%4)
        assert found
        emu.tap(0x32);frame('revision=2 dirty=1');request('save');offset=len(log());emu.tap(0x44);frame('revision=2 dirty=1 status=SAVE REFUSED',offset)
        assert (run/'new-project.ptg').read_bytes()==saved;capture('04-existing-file-refused.png')
        offset=len(log());chord(0x18);frame('status=UNSAVED EDITS: LOAD AGAIN',offset);capture('05-load-discard-confirmation.png')
        emu.tap(0x4d);request('load',dirty=True);offset=len(log());emu.tap(0x45);frame('revision=2 dirty=1 status=LOAD CANCELLED',offset)
        request('load',dirty=True);filename('bad.mod');offset=len(log());emu.tap(0x44);frame('revision=2 dirty=1 status=LOAD FAILED',offset);capture('06-invalid-load-preserved.png')
        request('load',dirty=True);filename('new-project.ptg');offset=len(log());emu.tap(0x44);frame('revision=0 dirty=0 status=PROJECT LOADED',offset)
        emu.tap(0x57);wait(lambda:'period=381,339,285,214' in log());capture('07-loaded-project-playing.png');emu.tap(0x59);frame('status=STOPPED - AUDIO RELEASED')
        request('save',shift=True);offset=len(log());emu.tap(0x45);frame('status=SAVE CANCELLED',offset)
        request('save');filename('copy.ptg');offset=len(log());emu.tap(0x44);frame('status=PROJECT SAVED',offset)
        assert (run/'copy.ptg').read_bytes()==saved;assert (run/'a.mod').read_bytes()==(ROOT/'evidence/baseline/mod.baseline').read_bytes()
        emu.tap(0x45);wait(lambda:(run/'done').exists());assert (run/'editor.rc').read_text().strip()=='0'
        benchmarks={name:(run/name).read_text() for name in ['view-after.log','view-before.log'] if (run/name).exists()}
        assert all('VIEW PASS' in value for value in benchmarks.values())
        assert 'identical=1' in benchmarks['view-after.log']
        cursor=dict(item.split('=') for item in benchmarks['view-after.log'].splitlines()[1].split()[2:])
        assert int(cursor['partial_ticks50'])<int(cursor['full_ticks50'])
        if args.previous_view_binary:
            assert benchmarks['view-after.log'].splitlines()[0].split('pixel_fnv=')[1]==benchmarks['view-before.log'].splitlines()[0].split('pixel_fnv=')[1]
        report={'view_benchmarks':benchmarks,'view_binaries':{name:digest(run/name) for name in ['PTViewBench','PTViewBefore'] if (run/name).exists()},'run_id':run.name,'binary_sha256':digest(run/'PT24GEdit'),'elapsed_seconds':round(time.monotonic()-start,3),'log':log(),
            'no_argument_blank_start':True,'native_load_save_dialogs':True,'saved_D2_and_crc_verified':True,'existing_file_preserved':True,
            'dirty_load_other_key_cancels_confirmation':True,'cancel_and_invalid_load_preserve_dirty_edits':True,'reloaded_playback_period_381':True,'resaved_byte_identity':True,'normal_exit':True,
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL']}}
        (out/'native-requesters.json').write_text(json.dumps(report,indent=2)+'\n');shutil.copyfile(run/'new-project.ptg',out/'saved.ptg')
        print('PASS: blank start, native load/save/cancel, discard confirmation, invalid-load and existing-file preservation, reload/replay and exact resave')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Requester test released:',run)
if __name__=='__main__':main()
