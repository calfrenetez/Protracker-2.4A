#!/usr/bin/env python3
"""Native PP20 MOD loading, atomic failure and classic export workflow; reserve the private Amiberry window first."""
import argparse
import hashlib
from make_pp20_fixture import literal
import io
import math
import json
from pathlib import Path
import shutil
import subprocess
import time
import wave
import zlib
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket


def wav(channels,width,rate,data):
    out=io.BytesIO()
    with wave.open(out,'wb') as f:
        f.setnchannels(channels);f.setsampwidth(width);f.setframerate(rate);f.writeframes(data)
    return out.getvalue()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--genuine-pp',type=Path,required=True);parser.add_argument('--genuine-mod',type=Path,required=True);args=parser.parse_args()
    genuine=args.genuine_pp.read_bytes();expected=args.genuine_mod.read_bytes()
    assert hashlib.sha256(genuine).hexdigest()=='1c97a8363ff0b8ddeba05e76e87409f5f56a9e6af83b07b7150402bec4e52a7f'
    assert hashlib.md5(expected).hexdigest()=='80ba11ca20f7ffef184a58c1fc619c18'
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('pp20'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/pp20-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PT24GConvert','PTPp20Test']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    plain=(ROOT/'evidence/baseline/mod.baseline').read_bytes();(run/'baseline.mod').write_bytes(plain);(run/'baseline.pp').write_bytes(literal(plain))
    (run/'genuine.pp').write_bytes(genuine);bad=bytearray(literal(plain));bad[4]=1;(run/'bad.pp').write_bytes(bad)
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
    def key(raw,control=False,shift=False,ack=True):
        offset=len(log())
        if control:emu.command('SEND_KEY',0x63,1)
        if shift:emu.command('SEND_KEY',0x60,1)
        try:emu.tap(raw)
        finally:
            if shift:emu.command('SEND_KEY',0x60,0)
            if control:emu.command('SEND_KEY',0x63,0)
        if ack:wait(lambda:'EDITOR FRAME' in log()[offset:] or 'EDITOR EXIT clean' in log()[offset:])
        return offset
    def request(kind):
        offset=key(0x18 if kind=='load' else 0x21 if kind=='save' else 0x37,control=True,shift=kind!='load',ack=False)
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
    def number(value):
        for digit in str(value):key(0x0a if digit=='0' else int(digit))
        return key(0x44)
    def samples(data):
        check=bytearray(data);check[20:24]=bytes(4);assert zlib.crc32(check)==int.from_bytes(data[20:24],'big')
        pos=32
        for _ in range(int.from_bytes(data[24:28],'big')):
            size=int.from_bytes(data[pos+8:pos+12],'big')
            if data[pos:pos+4]==b'SAMP':return data[pos+12:pos+12+size]
            pos+=12+size+(-size%4)
        raise AssertionError('Missing sample chunk')
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTPp20Test baseline.pp baseline.mod >pp20.log','Echo $RC >pp20.rc',
            'PT24GConvert mod genuine.pp converted.mod >convert.log','Echo $RC >convert.rc',
            'PT24GEdit baseline.pp edited.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit genuine.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['pp20','convert'])
        assert (run/'converted.mod').read_bytes()==expected;capture('01-packed-baseline.png')
        key(0x40);key(0x32);frame('revision=1 dirty=1');key(0x31,True);frame('revision=0 dirty=0 status=UNDO')
        request('load');filename('bad.pp');key(0x44);frame('revision=0 dirty=0 status=LOAD FAILED')
        capture('02-corrupt-load-preserved.png');key(0x31,True,True);frame('revision=1 dirty=1 status=REDO')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');assert (run/'edited.ptg').exists()
        request('load');filename('genuine.pp');key(0x44);frame('status=PROJECT LOADED');assert 'success channels=4 patterns=8' in log()
        capture('03-genuine-packed-mod.png')
        request('mod');filename('exact.mod');key(0x44);frame('MOD EXPORTED AND VERIFIED');assert (run/'exact.mod').read_bytes()==expected
        request('save');filename('genuine.ptg');key(0x44);frame('dirty=0 status=PROJECT SAVED')
        saved=(run/'genuine.ptg').read_bytes();samples(saved)
        key(0x45);current='reopened.log';frame('status=READY -')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');assert (run/'reopened.ptg').read_bytes()==saved
        key(0x45);wait(lambda:(run/'done').exists())
        assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened'])
        audio=audio_off();capture('05-normal-exit.png')
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PT24GConvert','PTPp20Test']},
            'native_pp20_atomic_tests':True,'genuine_fixture_sha256':hashlib.sha256(genuine).hexdigest(),
            'expected_decoded_md5':hashlib.md5(expected).hexdigest(),'decoded_bytes':len(expected),
            'native_converter_and_editor_exact_mod':True,'corrupt_load_preserves_redo':True,
            'exact_project_crc_and_reopen_identity':True,'normal_exits':2,'stopped_audio':audio,
            'logs':{n:(run/n).read_text() for n in ['pp20.log','convert.log','editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-pp20.json').write_text(json.dumps(report,indent=2)+'\n')
        # Third-party music and derived projects remain local; never copy them to tracked evidence.
        print('PASS: native genuine PP20, exact MOD via editor/converter, corrupt-load redo preservation, project reopen')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('PP20 test released:',run)
if __name__=='__main__':main()
