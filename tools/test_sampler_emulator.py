#!/usr/bin/env python3
"""Native sampler/ASL workflow; reserve the private Amiberry window first."""
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
    run=share/('sampler'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/sampler-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTSamplerTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'input.mod')
    mono=bytes((i*3)%256 for i in range(128));source=wav(1,1,8287,mono);(run/'a.wav').write_bytes(source)
    values=[((i*76543)%16777216)-8388608 for i in range(256)]
    stereo=b''.join(v.to_bytes(3,'little',signed=True) for v in values);high=wav(2,3,44100,stereo);(run/'b.wav').write_bytes(high)
    (run/'bad.wav').write_bytes(b'not a WAV');process=emu=None;start=time.monotonic();current='editor.log'
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
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTSamplerTest >sampler.log','Echo $RC >sampler.rc',
            'PT24GEdit input.mod saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert (run/'sampler.rc').read_text().strip()=='0'
        key(0x57);wait(lambda:'EDITOR REPLAY active=1' in log());key(0x28,True);frame('panel=5')
        request(True);capture('01-wav-import-requester.png');filename('a.wav');offset=key(0x44)
        frame('revision=1 dirty=1 status=SAMPLE UPDATED',offset);audio_off();capture('02-imported-mono-waveform.png')
        key(0x57);frame('SAMPLE AUDITION');wait(lambda:'period=428,0,0,0' in log())
        key(0x13);frame('revision=2 dirty=1');audio_off();capture('03-reversed-mono-waveform.png')
        key(0x17);frame('revision=3 dirty=1');key(0x31,True);frame('revision=2 dirty=1 status=UNDO')
        request(True);filename('bad.wav');offset=key(0x44);frame('revision=2 dirty=1 status=WAV FORMAT',offset)
        request(True);offset=key(0x45);frame('revision=2 dirty=1 status=SAMPLE FILE REQUEST CANCELLED',offset)
        key(0x31,True,True);frame('revision=3 dirty=1 status=REDO');key(0x31,True)
        request(False);filename('out.wav');offset=key(0x44);frame('WAV EXPORTED AND VERIFIED',offset)
        expected=wav(1,1,8287,mono[::-1]);assert (run/'out.wav').read_bytes()==expected
        request(False);offset=key(0x44);frame('WAV EXPORT REFUSED',offset);assert (run/'out.wav').read_bytes()==expected
        key(0x0c);request(True);filename('b.wav');offset=key(0x44);frame('SAMPLE UPDATED',offset)
        key(0x22);key(0x31,True);capture('04-stereo24-waveform.png')
        offset=key(0x57);frame('FORMAT NEEDS ENHANCED PREVIEW',offset);audio_off()
        request(False);filename('hi.wav');offset=key(0x44);frame('WAV EXPORTED AND VERIFIED',offset)
        assert (run/'hi.wav').read_bytes()==high
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');saved=(run/'saved.ptg').read_bytes()
        check=bytearray(saved);check[20:24]=bytes(4);assert zlib.crc32(check)==int.from_bytes(saved[20:24],'big')
        # Independent serialized sample inspection: signed big-endian PTG PCM.
        pos=32;sample_data=None
        for _ in range(int.from_bytes(saved[24:28],'big')):
            size=int.from_bytes(saved[pos+8:pos+12],'big')
            if saved[pos:pos+4]==b'SAMP':sample_data=saved[pos+12:pos+12+size]
            pos+=12+size+(-size%4)
        assert sample_data is not None
        assert sample_data[40:44]==bytes([8,1,64,0]) and sample_data[64:192]==bytes((v-128)&255 for v in mono[::-1])
        second=sample_data[192:];assert second[40:44]==bytes([24,2,64,0])
        assert second[64:64+768]==b''.join(v.to_bytes(3,'big',signed=True) for v in values)
        key(0x45);key(0x45);current='reopened.log';frame('status=READY -')
        key(0x28,True);key(0x0c);capture('05-reopened-stereo24.png')
        request(False);filename('copy.wav');offset=key(0x44);frame('WAV EXPORTED AND VERIFIED',offset)
        assert (run/'copy.wav').read_bytes()==high
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');assert (run/'reopened.ptg').read_bytes()==saved
        key(0x45);key(0x45);wait(lambda:(run/'done').exists())
        assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened'])
        audio=audio_off();capture('06-normal-exit.png')
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTSamplerTest']},
            'exact_mono_import_reverse_export':True,'exact_stereo24_import_export_and_project_bytes':True,'unsupported_high_resolution_audition_refused':True,
            'bad_and_cancelled_import_preserve_redo':True,'existing_wav_preserved':True,'edit_and_import_stop_owned_audio':True,
            'reopened_project_and_wav_byte_identity':True,'normal_exits':2,'stopped_audio':audio,
            'logs':{n:(run/n).read_text() for n in ['sampler.log','editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-sampler.json').write_text(json.dumps(report,indent=2)+'\n')
        for n in ['a.wav','b.wav','out.wav','hi.wav','saved.ptg']:shutil.copyfile(run/n,out/n)
        print('PASS: native WAV dialogs, mono/stereo24 import/edit/undo/export, exact project save/reopen, audio stop and cleanup')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Sampler test released:',run)
if __name__=='__main__':main()
