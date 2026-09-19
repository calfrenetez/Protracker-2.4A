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
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--layout-fixture",type=Path)
    parser.add_argument("--playback-check",action="store_true",help="Verify live 150 BPM text pixels and stopped DMA")
    args=parser.parse_args()
    if args.playback_check:
        from PIL import Image
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
    if args.playback_check:
        baseline=(ROOT/'evidence/baseline/mod.baseline').read_bytes()
        assert baseline[1080:1084]==b'M.K.' and baseline[952]==0
        tempo=bytearray(baseline);event=1084+8*16
        tempo[event+2]=(tempo[event+2]&0xf0)|0x0f;tempo[event+3]=150
        assert tempo[:event+2]==baseline[:event+2] and tempo[event+4:]==baseline[event+4:]
        assert tempo[event+2]&0xf0==baseline[event+2]&0xf0
        (run/'tempo.mod').write_bytes(tempo)
        shutil.copyfile(run/'tempo.mod',out/'tempo.mod')
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
    def dma_stopped():
        audio=emu.command('GET_AUDIO_STATE').split('\t')
        return all('ch%d_dma=0'%i in audio for i in range(4))
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PT24GEdit input.ptg saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc'] + (['PT24GEdit layout.ptg >layout.log','Echo $RC >layout.rc'] if args.layout_fixture else []) + (['PT24GEdit tempo.mod >tempo.log','Echo $RC >tempo.rc'] if args.playback_check else []) + ['Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as log:
            process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],
                stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        wait_for(lambda: bool(matching_socket()),45); matches=matching_socket()
        if len(matches)!=1: raise RuntimeError('Ambiguous emulator')
        emu=Emulator(matches[0])
        wait_for(lambda: (run/'editor.log').exists() and 'EDITOR READY' in (run/'editor.log').read_text(),45)
        frame('editor.log','revision=0 dirty=0 status=READY -'); capture('01-native-page4.png')
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
            frame('layout.log','revision=0 dirty=0 status=READY -');capture('09-reference-layout-native.png');emu.tap(0x45)
            wait_for(lambda:(run/'layout.rc').exists());assert (run/'layout.rc').read_text().strip()=='0'
        playback=None
        if args.playback_check:
            frame('tempo.log','revision=0 dirty=0 status=READY -')
            emu.tap(0x57)  # F8: no further editor input/redraw before the capture.
            wait_for(lambda:any(line.startswith('EDITOR REPLAY ') and 'active=1' in line.split() and
                'bpm=150' in line.split() for line in (run/'tempo.log').read_text().splitlines()))
            capture('10-live-tempo-native.png')
            # Independent pixel expectation from the pinned source font: three
            # 12-pixel glyphs at ten-pixel advances, doubled source scanlines.
            expected_bpm=Image.new('RGB',(32,10),(119,119,119))
            font=(ROOT/'vendor/pt23f/raw/ptfont.raw').read_bytes()
            for digit,char in enumerate('150'):
                for row in range(5):
                    bits=font[(ord(char)-32)*8+row]
                    for bit in range(8):
                        if bits&(128>>bit):
                            for x in range(bit*3//2,(bit+1)*3//2):
                                for y in range(row*2,row*2+2):expected_bpm.putpixel((digit*10+x,y),(0,17,68))
            with Image.open(out/'10-live-tempo-native.png') as screenshot:
                actual_bpm=screenshot.convert('RGB').crop((76+79,39+213,76+79+32,39+213+10))
            expected_bpm.save(out/'10-live-tempo-expected.png');actual_bpm.save(out/'10-live-tempo-actual.png')
            assert actual_bpm.tobytes()==expected_bpm.tobytes(), 'Live BPM pixels differ from the independently drawn 150'
            emu.tap(0x59)  # F10: release all four Paula DMA channels.
            wait_for(dma_stopped)
            stopped_audio=emu.command('GET_AUDIO_STATE')
            assert all('ch%d_dma=0'%i in stopped_audio.split('\t') for i in range(4))
            emu.tap(0x45);wait_for(lambda:(run/'tempo.rc').exists())
            assert (run/'tempo.rc').read_text().strip()=='0'
            tempo_log=(run/'tempo.log').read_text();assert 'EDITOR EXIT clean' in tempo_log
            shutil.copyfile(run/'tempo.log',out/'tempo.log')
            playback={'fixture_sha256':digest(run/'tempo.mod'),'effect_row':8,'effect_channel':0,
                'effect':'F96 (150 BPM)','sample_and_period_bytes_preserved':True,
                'live_bpm_pixels_match':True,'pixel_region':[155,252,32,10],
                'capture':'10-live-tempo-native.png','capture_sha256':digest(out/'10-live-tempo-native.png'),
                'stopped_audio':stopped_audio,'normal_exit':True,
                'no_editor_input_between_play_and_capture':True}
        wait_for(lambda:(run/'done').exists())
        assert (run/'reopened.rc').read_text().strip()=='0'
        capture('08-return-to-workbench.png')
        logs={name:(run/name).read_text() for name in ['editor.log','reopened.log']}
        if args.playback_check:logs['tempo.log']=(run/'tempo.log').read_text()
        assert 'EDITOR SAVE result=0 dirty=0' in logs['editor.log']
        assert 'EDITOR SAVE result=6 dirty=1' in logs['editor.log']
        assert 'EDITOR EXIT clean' in logs['editor.log'] and 'EDITOR EXIT clean' in logs['reopened.log']
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binary_sha256':digest(run/'PT24GEdit'),'saved_sha256':digest(run/'saved.ptg'),
            'native_edit_exact_expected_bytes':True,'existing_destination_preserved':True,
            'reopen_save_byte_identity':True,'normal_exit_twice':True,'layout_fixture_captured':bool(args.layout_fixture),'logs':logs,
            'playback_check':playback,
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
