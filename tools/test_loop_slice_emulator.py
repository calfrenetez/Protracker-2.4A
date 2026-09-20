#!/usr/bin/env python3
"""Native loop/slice workflow; reserve the private Amiberry window first."""
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
    run=share/('loops'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/loop-slice-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTSamplerTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'input.mod')
    signed=[0]*2048
    for base in [0,512,1024,1536]:
        for i in range(64):signed[base+i]=100 if i%4<2 else -100
    signed[-32:]=[-20]*32
    source=wav(1,1,8287,bytes(v+128 for v in signed));(run/'a.wav').write_bytes(source)
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
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTSamplerTest >sampler.log','Echo $RC >sampler.rc',
            'PT24GEdit input.mod saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert (run/'sampler.rc').read_text().strip()=='0'
        assert 'LOOP/SLICE PASS' in (run/'sampler.log').read_text()
        key(0x28,True);request(True);filename('a.wav');offset=key(0x44);frame('revision=1 dirty=1 status=SAMPLE UPDATED',offset)
        key(0x57);frame('SAMPLE AUDITION');key(0x42);frame('panel=6')
        key(0x23);frame('revision=2 dirty=1 status=LOOP UPDATED');audio_off()
        key(0x19);frame('revision=3 dirty=1');capture('01-pingpong-loop.png')
        offset=key(0x57);frame('FORMAT NEEDS ENHANCED PREVIEW',offset);audio_off()
        key(0x31,True);frame('revision=2 dirty=1 status=UNDO')
        key(0x31,True,True);frame('revision=3 dirty=1 status=REDO')
        key(0x18);frame('revision=4 dirty=1 status=LOOP UPDATED');key(0x31,True)
        key(0x35);frame('revision=5 dirty=1 status=CROSSFADE BAKED');capture('02-baked-forward-loop.png')
        key(0x31,True);frame('revision=3 dirty=1 status=UNDO');key(0x31,True,True);frame('revision=5 dirty=1 status=REDO')
        key(0x42);frame('panel=7');key(0x33);key(0x37);frame('revision=5 dirty=1 status=SLICE PROPOSAL')
        key(0x19);frame('revision=6 dirty=1 status=SLICE MARKERS APPLIED')
        key(0x31,True);key(0x14);capture('03-auto-proposal.png');key(0x32);frame('revision=5 dirty=1 status=SLICE PROPOSAL CANCELLED')
        key(0x31,True,True);frame('revision=6 dirty=1 status=REDO')
        key(0x14);key(0x22);key(0x37);capture('04-edited-proposal.png');key(0x19)
        frame('revision=7 dirty=1 status=SLICE MARKERS APPLIED');capture('05-saved-markers.png')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');saved=(run/'saved.ptg').read_bytes()
        check=bytearray(saved);check[20:24]=bytes(4);assert zlib.crc32(check)==int.from_bytes(saved[20:24],'big')
        pos=32;sample_data=None
        for _ in range(int.from_bytes(saved[24:28],'big')):
            size=int.from_bytes(saved[pos+8:pos+12],'big')
            if saved[pos:pos+4]==b'SAMP':sample_data=saved[pos+12:pos+12+size]
            pos+=12+size+(-size%4)
        assert sample_data is not None
        assert sample_data[40:46]==bytes([8,1,64,0,1,0])
        assert int.from_bytes(sample_data[48:52],'big')==32 and int.from_bytes(sample_data[52:56],'big')==2048
        assert int.from_bytes(sample_data[56:60],'big')==0
        count=int.from_bytes(sample_data[46:48],'big')
        markers=[int.from_bytes(sample_data[64+i*4:68+i*4],'big') for i in range(count)]
        assert markers==[0,512,1024,1536],markers
        expected=signed[:]
        for i in range(32):expected[2016+i]=int((signed[2016+i]*(31-i)+signed[i]*(i+1))/32)
        assert sample_data[64+4*count:64+4*count+2048]==bytes(v&255 for v in expected)
        key(0x45);key(0x45);current='reopened.log';frame('status=READY -')
        key(0x28,True);key(0x42);capture('06-reopened-loop.png');key(0x42);capture('07-reopened-markers.png')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');assert (run/'reopened.ptg').read_bytes()==saved
        key(0x45);key(0x45);wait(lambda:(run/'done').exists())
        assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened'])
        audio=audio_off();capture('08-normal-exit.png')
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTSamplerTest']},
            'loop_modes_and_baked_crossfade_undo_redo':True,'proposal_manual_edit_apply_cancel':True,
            'cancel_preserves_redo':True,'exact_forward_loop_and_baked_pcm_and_marker_bytes':True,'markers':markers,
            'pingpong_preview_refused':True,'sample_metadata_change_stops_audio':True,'project_crc_and_reopen_identity':True,
            'input':'real raw-key events; mouse targets covered by shared controller host tests','normal_exits':2,'stopped_audio':audio,
            'logs':{n:(run/n).read_text() for n in ['sampler.log','editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-loops-slices.json').write_text(json.dumps(report,indent=2)+'\n')
        for n in ['a.wav','saved.ptg','reopened.ptg']:shutil.copyfile(run/n,out/n)
        print('PASS: native loop/crossfade undo, editable slice proposals, exact saved metadata/PCM and reopen identity')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Loop/slice test released:',run)
if __name__=='__main__':main()
