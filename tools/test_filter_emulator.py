#!/usr/bin/env python3
"""Native filtered-resampling and cancellation workflow; reserve the private Amiberry window first."""
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
    run=share/('filter'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/filter-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTSamplerTest','PTFilterTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'input.mod')
    values=[round(12000*math.sin(2*math.pi*1000*i/32000)+12000*math.sin(2*math.pi*6000*i/32000)) for i in range(8192)]
    source=wav(1,2,32000,b''.join(v.to_bytes(2,'little',signed=True) for v in values));(run/'a.wav').write_bytes(source)
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
    def request(importing):
        offset=key(0x28 if importing else 0x11,ack=False)
        wait(lambda:('EDITOR REQUEST '+('sample' if importing else 'wav')) in log()[offset:]);time.sleep(.8)
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
            'PTFilterTest >filter.log','Echo $RC >filter.rc','PTSamplerTest >sampler.log','Echo $RC >sampler.rc',
            'PT24GEdit input.mod saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['filter','sampler'])
        key(0x28,True);request(True);filename('a.wav');key(0x44);frame('revision=1 dirty=1 status=SAMPLE UPDATED')
        key(0x33);key(3);key(0x19);frame('revision=2 dirty=1 status=SAMPLE UPDATED');key(0x31,True)
        key(2);key(0x13);number(8000);capture('01-filter-target.png')
        offset=key(0x19,ack=False);wait(lambda:'EDITOR CONVERSION progress=0' in log()[offset:])
        key(0x45);frame('revision=1 dirty=1 status=CONVERSION CANCELLED',offset);capture('02-cancelled.png')
        assert 'EDITOR CONVERSION cancelled=' in log()[offset:]
        key(0x42);request(False);filename('cancel.wav');key(0x44);frame('WAV EXPORTED AND VERIFIED')
        assert (run/'cancel.wav').read_bytes()==source
        key(0x31,True,True);frame('revision=2 dirty=1 status=REDO');key(0x31,True)
        key(0x33);key(0x13);number(8000)
        filtering_start=time.monotonic();offset=key(0x19,ack=False)
        wait(lambda:'EDITOR CONVERSION progress=0' in log()[offset:]);capture('03-filter-progress.png')
        frame('revision=3 dirty=1 status=SAMPLE UPDATED',offset);elapsed_filter=round(time.monotonic()-filtering_start,3)
        capture('04-filtered-waveform.png');assert 'EDITOR CONVERSION progress=100' in log()[offset:]
        key(0x42);request(False);filename('out.wav');key(0x44);frame('WAV EXPORTED AND VERIFIED')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');saved=(run/'saved.ptg').read_bytes();dst=samples(saved)
        assert dst[40:44]==bytes([16,1,64,0]) and int.from_bytes(dst[32:36],'big')==8000 and int.from_bytes(dst[36:40],'big')==2048
        assert int.from_bytes(dst[46:48],'big')==0
        actual=[int.from_bytes(dst[i:i+2],'big',signed=True) for i in range(64,64+4096,2)]
        errors=[abs(actual[i]-round(12000*math.sin(2*math.pi*1000*i/8000))) for i in range(32,2016)]
        assert max(errors)<=8,max(errors)
        assert (run/'out.wav').read_bytes()==wav(1,2,8000,b''.join(v.to_bytes(2,'little',signed=True) for v in actual))
        key(0x45);key(0x45);current='reopened.log';frame('status=READY -')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');assert (run/'reopened.ptg').read_bytes()==saved
        key(0x45);wait(lambda:(run/'done').exists())
        assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened'])
        audio=audio_off();capture('05-normal-exit.png')
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'filter_workflow_seconds':elapsed_filter,
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTSamplerTest','PTFilterTest']},
            'native_filter_tones_precision_boundaries':True,'escape_cancels_preserving_exact_source_and_redo':True,
            'progress_updates':True,'mixed_tone_output_max_error_lsb':max(errors),
            'exact_project_wav_and_reopen_identity':True,'normal_exits':2,'stopped_audio':audio,
            'logs':{n:(run/n).read_text() for n in ['filter.log','sampler.log','editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-filter.json').write_text(json.dumps(report,indent=2)+'\n')
        for n in ['a.wav','cancel.wav','out.wav','saved.ptg','reopened.ptg']:shutil.copyfile(run/n,out/n)
        print('PASS: native filter quality, cancellable progress, exact source/redo preservation, verified WAV/project and reopen')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Filtered conversion test released:',run)
if __name__=='__main__':main()
