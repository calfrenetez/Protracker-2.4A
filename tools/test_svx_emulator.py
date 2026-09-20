#!/usr/bin/env python3
"""Native IFF/8SVX import/export and refusal workflow; reserve the private Amiberry window first."""
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
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('svx'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/svx-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTSamplerTest','PTSvxTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'input.mod')
    def chunk(tag,data):return tag+len(data).to_bytes(4,'big')+data+bytes(len(data)%2)
    def iff(data,compression=0,name=b'IFF NATIVE',start=128,end=3072):
        vh=start.to_bytes(4,'big')+(end-start).to_bytes(4,'big')+bytes(4)+(8287).to_bytes(2,'big')+bytes([1,compression])+(49152).to_bytes(4,'big')
        body=b'8SVX'+chunk(b'VHDR',vh)+chunk(b'NAME',name)+chunk(b'BODY',data)
        return b'FORM'+len(body).to_bytes(4,'big')+body
    source=bytes((i%256) for i in range(4097));(run/'a.iff').write_bytes(iff(source))
    compressed=iff(bytes([0,120,255,8]),1,b'FIBONACCI',0,4);(run/'fib.iff').write_bytes(compressed)
    bad=bytearray(iff(source));bad[35]=2;(run/'bad.iff').write_bytes(bad)
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
        offset=key(0x28 if kind=='sample' else 0x11,shift=kind=='iff',ack=False)
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
            'PTSvxTest >svx.log','Echo $RC >svx.rc','PTSamplerTest >sampler.log','Echo $RC >sampler.rc',
            'PT24GEdit input.mod saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['svx','sampler'])
        key(0x28,True);request('sample');filename('a.iff');key(0x44);frame('revision=1 dirty=1 status=SAMPLE UPDATED')
        capture('01-imported-iff.png')
        request('iff');filename('out.iff');key(0x44);frame('IFF EXPORTED AND VERIFIED')
        exported=(run/'out.iff').read_bytes();assert exported[:12]==b'FORM'+(88+len(source)+1-8).to_bytes(4,'big')+b'8SVX'
        assert exported[20:40]==(run/'a.iff').read_bytes()[20:40]
        assert exported[48:80]==b'IFF NATIVE'+bytes(22) and exported[88:]==source+b'\0'
        key(0x42);key(0x19);frame('revision=2 dirty=1 status=LOOP UPDATED')
        offset=key(0x11);frame('IFF NEEDS MONO8',offset);assert 'EDITOR REQUEST' not in log()[offset:]
        key(0x31,True);frame('revision=1 dirty=1 status=UNDO')
        key(0x42);key(0x42);key(0x42);request('sample');filename('bad.iff');key(0x44);frame('revision=1 dirty=1 status=SAMPLE FORMAT OR SLICE')
        key(0x31,True,True);frame('revision=2 dirty=1 status=REDO');key(0x31,True)
        request('sample');filename('fib.iff');key(0x44);frame('revision=3 dirty=1 status=SAMPLE UPDATED')
        request('wav');filename('fib.wav');key(0x44);frame('WAV EXPORTED AND VERIFIED')
        assert (run/'fib.wav').read_bytes()==wav(1,1,8287,bytes([13,34,0,0]))
        capture('02-compressed-import.png');key(0x31,True);frame('revision=1 dirty=1 status=UNDO')
        request('iff');filename('out.iff');key(0x44);frame('IFF EXPORT REFUSED OR FAILED')
        assert (run/'out.iff').read_bytes()==exported
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');saved=(run/'saved.ptg').read_bytes();dst=samples(saved)
        assert dst[:32]==b'IFF NATIVE'+bytes(22) and dst[40:46]==bytes([8,1,48,0,1,0])
        assert int.from_bytes(dst[32:36],'big')==8287 and int.from_bytes(dst[36:40],'big')==4097
        assert dst[48:56]==(128).to_bytes(4,'big')+(3072).to_bytes(4,'big') and dst[64:64+4097]==source
        capture('03-saved-project.png')
        key(0x45);key(0x45);current='reopened.log';frame('status=READY -')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');assert (run/'reopened.ptg').read_bytes()==saved
        key(0x45);wait(lambda:(run/'done').exists())
        assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened'])
        audio=audio_off();capture('05-normal-exit.png')
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTSamplerTest','PTSvxTest']},
            'native_codec_and_sampler_tests':True,'exact_name_volume_forward_loop_pcm':True,
            'fibonacci_import_exact_wav':True,'malformed_import_preserves_redo':True,
            'unsupported_export_refuses_before_requester':True,'existing_destination_preserved':True,
            'exact_project_crc_and_reopen_identity':True,'normal_exits':2,'stopped_audio':audio,
            'logs':{n:(run/n).read_text() for n in ['svx.log','sampler.log','editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-svx.json').write_text(json.dumps(report,indent=2)+'\n')
        for n in ['a.iff','fib.iff','bad.iff','out.iff','fib.wav','saved.ptg','reopened.ptg']:shutil.copyfile(run/n,out/n)
        print('PASS: native IFF metadata/PCM, Fibonacci import, refusal/redo, verified export/project and reopen')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('IFF conversion test released:',run)
if __name__=='__main__':main()
