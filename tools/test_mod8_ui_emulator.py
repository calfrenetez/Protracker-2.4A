#!/usr/bin/env python3
"""Explicit native MOD8 UI, cancellation and source/history preservation. Reserve first."""
import json,shutil,subprocess,time,zlib
from pathlib import Path
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket

def main():
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    manifest=json.loads((ROOT/'build/dev/core-build.json').read_text());assert all(digest(ROOT/p)==h for p,h in manifest['sources'].items())
    assert digest(ROOT/'build/dev/PT24GEdit')==manifest['binaries']['PT24GEdit']['sha256']
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('mod8ui'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/mod8-ui-evidence'/run.name;out.mkdir(parents=True)
    shutil.copyfile(ROOT/'build/dev/PT24GEdit',run/'PT24GEdit')
    source=ROOT/'evidence/enhanced-editor/dev59/native/high.ptg';shutil.copyfile(source,run/'high.ptg')
    mixed=ROOT/'tests/fixtures/project-v1/mixed.ptg';shutil.copyfile(mixed,run/'mixed.ptg')
    metadata=bytearray(source.read_bytes());pos=32
    for _ in range(int.from_bytes(metadata[24:28],'big')):
        size=int.from_bytes(metadata[pos+8:pos+12],'big');body=pos+12
        if metadata[pos:pos+4]==b'SAMP':metadata[body+45]=1
        pos=body+size+(-size%4)
    metadata[20:24]=bytes(4);metadata[20:24]=zlib.crc32(metadata).to_bytes(4,'big');(run/'metadata.ptg').write_bytes(metadata)
    expected=bytearray((ROOT/'evidence/enhanced-editor/dev59/native/expected.mod').read_bytes());assert expected[44]==0;expected[44]=1
    process=emu=None;start=time.monotonic();current='editor.log'
    def log():return (run/current).read_text() if (run/current).exists() else ''
    def wait(check,seconds=45):
        until=min(start+300,time.monotonic()+seconds)
        while time.monotonic()<until:
            if check():return
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            time.sleep(.1)
        raise RuntimeError('MOD8 UI deadline '+str(run)+' '+log()[-600:])
    def frame(text,offset=0):wait(lambda:any('EDITOR FRAME' in s and text in s for s in log()[offset:].splitlines()))
    def key(raw,control=False,alt=False,shift=False,ack=True):
        offset=len(log())
        for active,code in [(control,0x63),(alt,0x64),(shift,0x60)]:
            if active:emu.command('SEND_KEY',code,1)
        try:emu.tap(raw)
        finally:
            for active,code in [(shift,0x60),(alt,0x64),(control,0x63)]:
                if active:emu.command('SEND_KEY',code,0)
        if ack:wait(lambda:'EDITOR FRAME' in log()[offset:] or 'EDITOR EXIT clean' in log()[offset:])
        return offset
    def request():
        offset=key(0x44,ack=False);wait(lambda:'EDITOR REQUEST mod8' in log()[offset:]);time.sleep(.8);return offset
    def filename(text):
        keys=dict(zip('abcdefghijklmnopqrstuvwxyz',[0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31]));keys['.']=0x39
        emu.command('SEND_KEY',0x60,1)
        try:emu.tap(0x4f);emu.tap(0x46)
        finally:emu.command('SEND_KEY',0x60,0)
        for ch in text:emu.tap(keys[ch])
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PT24GEdit high.ptg saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit mixed.ptg >mixed.log','Echo $RC >mixed.rc',
            'PT24GEdit metadata.ptg >metadata.log','Echo $RC >metadata.rc','Echo '+run.name+' >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');key(0x4c,alt=True);frame('revision=1 dirty=1 status=SAMPLE UPDATED')
        offset=key(0x37,control=True,shift=True);frame('MOD EXPORT REFUSED: SAMPLE FORMAT',offset);assert 'EDITOR REQUEST mod' not in log()[offset:]
        key(0x37,control=True,alt=True);frame('8 BIT ROUND / NO DITHER')
        shot=out/(run.name+'.png');emu.command('SCREENSHOT',shot);wait(lambda:shot.exists() and shot.read_bytes().endswith(b'\0\0\0\0IEND\xaeB`\x82'));shot.rename(out/'conversion-panel.png')
        offset=request();key(0x45);frame('revision=1 dirty=1 status=MOD EXPORT CANCELLED',offset)
        offset=request();filename('converted.mod');key(0x44,ack=False);frame('revision=1 dirty=1 status=MOD CONVERTED - UNSAVED',offset)
        assert (run/'converted.mod').read_bytes()==expected
        offset=request();key(0x44,ack=False);frame('MOD EXPORT REFUSED: DESTINATION EXISTS',offset);assert (run/'converted.mod').read_bytes()==expected
        key(0x45);key(0x31,control=True);frame('revision=0 dirty=0 status=UNDO');key(0x21,control=True);frame('PROJECT SAVED')
        assert (run/'saved.ptg').read_bytes()==source.read_bytes();key(0x45)
        current='mixed.log';frame('status=READY -');key(0x37,control=True,alt=True);offset=key(0x44);frame('MOD EXPORT REFUSED: EXTERNAL MIDI',offset)
        assert 'EDITOR REQUEST mod8' not in log();key(0x45);key(0x45)
        current='metadata.log';frame('status=READY -');key(0x37,control=True,alt=True);offset=key(0x44);frame('MOD EXPORT REFUSED: ENHANCED DATA',offset)
        assert 'EDITOR REQUEST mod8' not in log();key(0x45);key(0x45);wait(lambda:(run/'done').exists())
        for n in ['editor','mixed','metadata']:assert (run/(n+'.rc')).read_text().strip()=='0'
        assert (run/'high.ptg').read_bytes()==source.read_bytes() and (run/'mixed.ptg').read_bytes()==mixed.read_bytes()
        assert not list(run.glob('*.pttmp-*'))
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'binary':manifest['binaries']['PT24GEdit'],'source_sha256':digest(source),'exact_converted_output':True,'cancel_existing_destination_dirty_undo_preserved':True,'strict_MIDI_and_remaining_metadata_refused':True,'stopped_audio':audio,'physical_tested':False,'logs':{n:(run/n).read_text() for n in ['editor.log','mixed.log','metadata.log']}}
        for n in ['converted.mod','saved.ptg']:shutil.copyfile(run/n,out/n)
        (out/'native-mod8-ui.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS: '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            if emu is None:
                matches=matching_socket()
                if len(matches)==1:emu=Emulator(matches[0])
            if emu is None:raise RuntimeError('Retain emulator claim: no guarded socket')
            Emulator(emu.path).command('QUIT');process.wait(timeout=10)
if __name__=='__main__':main()
