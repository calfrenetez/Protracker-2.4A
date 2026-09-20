#!/usr/bin/env python3
"""Native range/format workflow; reserve the private Amiberry window first."""
import io
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
    run=share/('format'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/range-format-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTSamplerTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    shutil.copyfile(ROOT/'evidence/enhanced-editor/dev14/loops-slices/saved.ptg',run/'input.ptg')
    process=emu=None;start=time.monotonic();current='editor.log'
    def log():return (run/current).read_text() if (run/current).exists() else ''
    def wait(check,seconds=50):
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
            'PTSamplerTest >sampler.log','Echo $RC >sampler.rc',
            'PT24GEdit input.ptg saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert (run/'sampler.rc').read_text().strip()=='0'
        assert 'CONVERSION PASS' in (run/'sampler.log').read_text()
        key(0x28,True);key(0x42);key(0x42);key(0x42);frame('panel=8')
        key(0x21);number(123);key(0x12);number(456);frame('revision=0 dirty=0 status=EXACT FRAME RANGE SET')
        key(0x34);capture('01-exact-selection-zoom.png')
        key(0x17);key(0x4e);capture('02-zoom-and-pan.png');key(0x18)
        key(0x21);number(2048);frame('INVALID FRAME RANGE');key(0x45)
        key(0x12);number(4294967295);frame('INVALID FRAME RANGE');key(0x45)
        key(0x34);key(0x42);key(0x42);frame('panel=6');key(0x23);frame('revision=1 dirty=1 status=LOOP UPDATED')
        capture('03-exact-loop.png');key(0x42);key(0x42);key(0x33);frame('panel=9')
        key(2);key(0x13);number(192001);frame('INVALID RATE');key(0x45)
        key(0x13);number(16574);capture('04-format-target.png')
        key(0x19);frame('revision=2 dirty=1 status=SAMPLE UPDATED');capture('05-converted.png')
        key(0x31,True);frame('revision=1 dirty=1 status=UNDO')
        key(0x13);number(1);key(0x19);frame('revision=1 dirty=1 status=CONVERSION WOULD COLLAPSE')
        key(0x31,True,True);frame('revision=2 dirty=1 status=REDO');key(0x42);key(0x33)
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');saved=(run/'saved.ptg').read_bytes()
        src=samples((run/'input.ptg').read_bytes());dst=samples(saved)
        assert dst[40:46]==bytes([16,1,64,0,1,0])
        assert int.from_bytes(dst[32:36],'big')==16574 and int.from_bytes(dst[36:40],'big')==4096
        assert int.from_bytes(dst[48:52],'big')==246 and int.from_bytes(dst[52:56],'big')==912
        count=int.from_bytes(dst[46:48],'big');assert count==4
        markers=[int.from_bytes(dst[64+i*4:68+i*4],'big') for i in range(count)];assert markers==[0,1024,2048,3072]
        old=[int.from_bytes(src[i:i+1],'big',signed=True) for i in range(80,80+2048)]
        expected=[]
        for i in range(4096):
            a=old[i//2];b=old[min(i//2+1,2047)]
            expected.append((a if i%2==0 else int((a+b)/2))*256)
        actual=[int.from_bytes(dst[i:i+2],'big',signed=True) for i in range(80,80+4096*2,2)]
        assert actual==expected
        key(0x45);key(0x45);current='reopened.log';frame('status=READY -')
        key(0x28,True);key(0x33);capture('06-reopened-format.png')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');assert (run/'reopened.ptg').read_bytes()==saved
        key(0x45);key(0x45);wait(lambda:(run/'done').exists())
        assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened'])
        audio=audio_off();capture('07-normal-exit.png')
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTSamplerTest']},
            'zoom_pan_and_exact_numeric_range':True,'view_changes_leave_project_clean':True,'invalid_range_and_rate_preserved':True,
            'exact_scaled_loops_markers_and_linear_pcm':True,'collapse_refusal_preserves_redo':True,
            'project_crc_and_reopen_identity':True,'markers':markers,'normal_exits':2,'stopped_audio':audio,
            'input':'real raw-key events; mouse targets and zoomed selection covered by shared controller host tests',
            'logs':{n:(run/n).read_text() for n in ['sampler.log','editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-range-format.json').write_text(json.dumps(report,indent=2)+'\n')
        for n in ['input.ptg','saved.ptg','reopened.ptg']:shutil.copyfile(run/n,out/n)
        print('PASS: native zoom/exact ranges, rate/precision conversion, scaled markers/loops, refusal/redo and exact save-reopen')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Range/format test released:',run)
if __name__=='__main__':main()
