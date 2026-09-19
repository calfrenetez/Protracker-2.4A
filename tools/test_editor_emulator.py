#!/usr/bin/env python3
"""Requires an explicitly reserved emulator window; tests the real IDCMP UI."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import time
import zlib
from build_diagnostic import ROOT, digest
from test_diagnostic_emulator import matching_socket
from emulator_ipc import Emulator


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--layout-fixture",type=Path);args=parser.parse_args()
    if matching_socket() or Path('/tmp/amiberry.sock').exists():
        raise SystemExit('An emulator socket exists; acquire exclusive ownership first')
    env = json.loads((ROOT/'local/environment.json').read_text())
    share = Path(env['share']); launch = share/'launch'; original = launch.read_bytes()
    run = share/('editor'+str(time.time_ns())); run.mkdir()
    out = ROOT/'build/dev/editor-evidence'; out.mkdir(exist_ok=True)
    shutil.copyfile(ROOT/'build/dev/PT24GEdit', run/'PT24GEdit')
    source = (ROOT/'tests/fixtures/project-v1/mixed.ptg').read_bytes()
    (run/'input.ptg').write_bytes(source)
    if args.layout_fixture:shutil.copyfile(args.layout_fixture,run/'layout.ptg')
    process = emu = None
    start = time.monotonic()
    def wait_for(condition, seconds=30):
        until = time.monotonic()+seconds
        while time.monotonic()<until:
            if condition(): return
            if process.poll() is not None: raise RuntimeError('Emulator exited unexpectedly')
            time.sleep(.15)
        raise RuntimeError('Editor test deadline expired: '+str(run))
    def chord(code, shift=False):
        emu.command('SEND_KEY',0x63,1)
        if shift: emu.command('SEND_KEY',0x60,1)
        try: emu.tap(code)
        finally:
            emu.command('SEND_KEY',0x63,0)
            if shift: emu.command('SEND_KEY',0x60,0)
        time.sleep(.3)
    def frame(log, text):
        wait_for(lambda: (run/log).exists() and any('EDITOR FRAME' in line and text in line for line in (run/log).read_text().splitlines()))
    def capture(name):
        time.sleep(.2); emu.command('SCREENSHOT',out/name)
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PT24GEdit input.ptg saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc'] + (['PT24GEdit layout.ptg >layout.log','Echo $RC >layout.rc'] if args.layout_fixture else []) + ['Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as log:
            process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],
                stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        wait_for(lambda: bool(matching_socket()),45); matches=matching_socket()
        if len(matches)!=1: raise RuntimeError('Ambiguous emulator')
        emu=Emulator(matches[0])
        wait_for(lambda: (run/'editor.log').exists() and 'EDITOR READY' in (run/'editor.log').read_text(),45)
        frame('editor.log','revision=0 dirty=0 status=EDITOR DEVELOPMENT'); capture('01-native-page4.png')
        emu.tap(0x50); emu.tap(0x40); emu.tap(0x31)  # F1, edit, Z
        chord(0x21); wait_for(lambda: (run/'saved.ptg').exists())
        saved=(run/'saved.ptg').read_bytes()
        expected=bytearray(source); pos=32
        for _ in range(int.from_bytes(source[24:28],'big')):
            tag=source[pos:pos+4]; n=int.from_bytes(source[pos+8:pos+12],'big'); body=pos+12
            if tag==b'HEAD': expected[body+33]=0  # selected channel
            if tag==b'PATT':
                expected[body:body+12]=bytes([1,1,1,172,0,0,0,0,0,0,0,0]) # C-2 / instrument 1 / no slice
            pos=body+n+(-n%4)
        expected[20:24]=b'\0'*4; expected[20:24]=zlib.crc32(expected).to_bytes(4,'big')
        assert saved==expected, 'Native edit/save differs from independently specified event and CRC'
        frame('editor.log','revision=1 dirty=0 status=PROJECT SAVED'); capture('02-native-note-saved.png')
        chord(0x31);frame('editor.log','revision=0 dirty=1 status=UNDO');capture('03-native-undo.png')
        chord(0x31,True);frame('editor.log','revision=1 dirty=0 status=REDO');capture('04-native-redo.png')
        emu.tap(0x53)
        for _ in range(3): emu.tap(0x42)
        emu.tap(0x31); chord(0x21)  # MIDI note, refused overwrite must retain dirty state
        frame('editor.log','revision=2 dirty=1 status=SAVE REFUSED');capture('05-native-existing-file-refused.png')
        assert (run/'saved.ptg').read_bytes()==saved
        emu.tap(0x45);frame('editor.log','status=UNSAVED EDITS');capture('06-native-discard-confirmation.png')
        emu.tap(0x45)
        wait_for(lambda:(run/'editor.rc').exists())
        assert (run/'editor.rc').read_text().strip()=='0'
        wait_for(lambda:(run/'reopened.log').exists() and 'EDITOR READY' in (run/'reopened.log').read_text())
        chord(0x21);wait_for(lambda:(run/'reopened.ptg').exists())
        assert (run/'reopened.ptg').read_bytes()==saved
        frame('reopened.log','revision=0 dirty=0 status=PROJECT SAVED');capture('07-native-project-reopened.png');emu.tap(0x45)
        if args.layout_fixture:
            frame('layout.log','revision=0 dirty=0 status=EDITOR DEVELOPMENT');capture('09-reference-layout-native.png');emu.tap(0x45)
            wait_for(lambda:(run/'layout.rc').exists());assert (run/'layout.rc').read_text().strip()=='0'
        wait_for(lambda:(run/'done').exists())
        assert (run/'reopened.rc').read_text().strip()=='0'
        capture('08-return-to-workbench.png')
        logs={name:(run/name).read_text() for name in ['editor.log','reopened.log']}
        assert 'EDITOR SAVE result=0 dirty=0' in logs['editor.log']
        assert 'EDITOR SAVE result=6 dirty=1' in logs['editor.log']
        assert 'EDITOR EXIT clean' in logs['editor.log'] and 'EDITOR EXIT clean' in logs['reopened.log']
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binary_sha256':digest(run/'PT24GEdit'),'saved_sha256':digest(run/'saved.ptg'),
            'native_edit_exact_expected_bytes':True,'existing_destination_preserved':True,
            'reopen_save_byte_identity':True,'normal_exit_twice':True,'layout_fixture_captured':bool(args.layout_fixture),'logs':logs,
            'environment':{c:emu.command(c) for c in ['GET_VERSION','GET_STATUS','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-editor.json').write_text(json.dumps(report,indent=2)+'\n')
        shutil.copyfile(run/'saved.ptg',out/'saved.ptg')
        print('PASS: native editor input, exact save, existing-file refusal, reopen and normal exits; inspect screenshots for UI acceptance')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu: Emulator(emu.path).command('QUIT')
                else: process.terminate()
            finally:
                try:process.wait(timeout=5)
                except subprocess.TimeoutExpired:process.terminate()


if __name__=='__main__': main()
