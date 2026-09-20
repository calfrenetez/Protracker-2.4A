#!/usr/bin/env python3
"""Native selected-instrument MOD source browsing and import workflow; reserve the private Amiberry window first."""
from make_mod_sample_fixture import make
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
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('source'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/source-evidence';out.mkdir(exist_ok=True)
    for name in ['PT24GEdit','PTSourceTest']:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'input.mod')
    baseline=(run/'input.mod').read_bytes();donor=make(baseline);(run/'donor.mod').write_bytes(donor);(run/'donor.pp').write_bytes(literal(donor))
    bad=bytearray(literal(donor));bad[4]=1;(run/'bad.pp').write_bytes(bad)
    expected_mod=bytearray(baseline);expected_mod[80:110]=donor[50:80];expected_mod.extend(donor[-32:])
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
        offset=key(0x37 if kind=='mod' else 0x28,control=kind=='mod',shift=kind=='mod',ack=False)
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
            'PTSourceTest donor.mod >source.log','Echo $RC >source.rc',
            'PT24GEdit input.mod saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc','Echo done >done'])+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['source'])
        key(0x28,True);request('sample');filename('donor.pp');key(0x44);frame('revision=0 dirty=0 status=MOD SOURCE READY')
        capture('01-packed-mod-source.png')
        request('source');offset=key(0x45);frame('SAMPLE FILE REQUEST CANCELLED',offset)
        request('source');filename('donor.mod');key(0x44);frame('revision=0 dirty=0 status=MOD SOURCE READY')
        key(0x4e);key(0x4e);key(0x44);frame('revision=0 dirty=0 status=SELECT A NONEMPTY SOURCE')
        key(0x4f);key(0x0c);key(0x0c);capture('02-selected-source-and-destination.png')
        key(0x17);frame('revision=1 dirty=1 status=SOURCE INSTRUMENT IMPORTED')
        key(0x31,True);frame('revision=0 dirty=0 status=UNDO')
        request('source');filename('bad.pp');key(0x44);frame('revision=0 dirty=0 status=SAMPLE FORMAT OR SLICE')
        key(0x31,True,True);frame('revision=1 dirty=1 status=REDO');key(0x45);frame('MOD SOURCE CLOSED')
        capture('03-imported-sample.png')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');saved=(run/'saved.ptg').read_bytes();dst=samples(saved)
        pos=0;records=[]
        for index in range(31):
            frames=int.from_bytes(dst[pos+36:pos+40],'big');count=int.from_bytes(dst[pos+46:pos+48],'big');size=64+count*4+frames*dst[pos+41]*(dst[pos+40]//8)
            records.append(dst[pos:pos+size]);pos+=size+(-size%4)
        record=records[2];assert record[:32]==b'SOURCE TWO'+bytes(22) and record[40:46]==bytes([8,1,48,253,1,0])
        assert record[48:56]==(8).to_bytes(4,'big')+(24).to_bytes(4,'big') and record[64:]==donor[-32:]
        request('mod');filename('exact.mod');key(0x44);frame('MOD EXPORTED AND VERIFIED');assert (run/'exact.mod').read_bytes()==expected_mod
        key(0x45);key(0x45);current='reopened.log';frame('status=READY -')
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');assert (run/'reopened.ptg').read_bytes()==saved
        key(0x45);wait(lambda:(run/'done').exists())
        assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened'])
        audio=audio_off();capture('05-normal-exit.png')
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTSourceTest']},
            'native_source_ownership_fault_tests':True,'plain_and_pp20_source_preview':True,
            'browse_and_cancel_preserve_song':True,'empty_source_refused':True,
            'selected_source2_to_destination3_exact':True,'invalid_source_preserves_preview_and_redo':True,
            'whole_song_exact_mod_comparison':True,'exact_project_crc_and_reopen_identity':True,'normal_exits':2,'stopped_audio':audio,
            'logs':{n:(run/n).read_text() for n in ['source.log','editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-source.json').write_text(json.dumps(report,indent=2)+'\n')
        for n in ['donor.mod','donor.pp','saved.ptg','reopened.ptg','exact.mod']:shutil.copyfile(run/n,out/n)
        print('PASS: native MOD/PP20 source preview, selected instrument import, exact metadata/PCM, song preservation, undo and reopen')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('MOD source test released:',run)
if __name__=='__main__':main()
