#!/usr/bin/env python3
"""Bounded donor UI regression; reserve shared030/DevBench before running.

For each request-NN-pointer.png, inspect that the pointer is on Load/Save, then
create request-NN-pointer-approved beside it within 120 seconds. This is a
harness operator check, not a request for another user approval.
"""
import argparse
import io
import wave
import fcntl
import subprocess
import sys
from make_mod_sample_fixture import make
from make_pp20_fixture import literal
import json
from pathlib import Path
import shutil
import time
import zlib
from build_diagnostic import ROOT,digest


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--precision',action='store_true',help='Exercise exact stereo24 sample import/edit/export/reopen')
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build/dev')
    args=parser.parse_args()
    infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
    sys.path.insert(0,str(infra/'scripts'))
    from shared_guest import Guest
    lock=(infra/'runtime/test.lock').open('a')
    fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    out=ROOT/'build/dev'/('source-shared-'+str(time.time_ns()));out.mkdir()
    guest=Guest(infra,out);share=guest.share;launch=guest.launch
    run=share/out.name;run.mkdir();finished=False;launched=False;requests=0
    for name in ['PT24GEdit','PTSourceTest']:shutil.copyfile(args.build_dir/name,run/name)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'input.mod')
    baseline=(run/'input.mod').read_bytes();donor=make(baseline);(run/'donor.mod').write_bytes(donor);(run/'donor.pp').write_bytes(literal(donor))
    bad=bytearray(literal(donor));bad[4]=1;(run/'bad.pp').write_bytes(bad)
    expected_mod=bytearray(baseline);expected_mod[80:110]=donor[50:80];expected_mod.extend(donor[-32:])
    values=[((i*76543)%16777216)-8388608 for i in range(256)]
    pcm=b''.join(v.to_bytes(3,'little',signed=True) for v in values)
    stream=io.BytesIO()
    with wave.open(stream,'wb') as w:
        w.setnchannels(2);w.setsampwidth(3);w.setframerate(44100);w.writeframes(pcm)
    high=stream.getvalue();(run/'high.wav').write_bytes(high);(run/'bad.wav').write_bytes(b'invalid sample')
    emu=guest;start=time.monotonic();current='editor.log'
    def log():return (run/current).read_text() if (run/current).exists() else ''
    def wait(check,seconds=30):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            if check():return
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
        offset=key(0x37 if kind=='mod' else 0x11 if kind=='wav' else 0x28,control=kind=='mod',shift=kind=='mod',ack=False)
        wait(lambda:('EDITOR REQUEST '+kind) in log()[offset:]);time.sleep(.8)
    def filename(text):
        nonlocal requests
        requests+=1
        emu.command('SCREENSHOT',out/('request-%02d-before.png'%requests))
        keys=dict(zip('abcdefghijklmnopqrstuvwxyz',[0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31]));keys.update({'.':0x39,'/':0x3a,'-':0x0b,':':0x29});keys.update({str(i):i for i in range(1,10)});keys['0']=0x0a
        text=(guest.device+run.name+'/'+text).lower()
        emu.command('SEND_KEY',0x60,1)
        try:emu.tap(0x4f);emu.tap(0x46)
        finally:emu.command('SEND_KEY',0x60,0)
        for ch in text:
            if ch==':':emu.command('SEND_KEY',0x60,1)
            try:emu.tap(keys[ch])
            finally:
                if ch==':':emu.command('SEND_KEY',0x60,0)
        emu.command('SCREENSHOT',out/('request-%02d-filled.png'%requests))
    def submit_request():
        offset=len(log())
        # Shared030 screenshot: display origin (76,40), Load center (188,440).
        # SEND_MOUSE deltas are display pixels here; legacy tracker calibration
        # undershoots this ASL requester. Bound deltas to avoid counter overflow.
        for _ in range(12):
            emu.command('SEND_MOUSE',-60,-60,0);time.sleep(.06)
        dx,dy=109,397  # pointer clamps three pixels inside the display origin
        while dx or dy:
            sx,sy=min(dx,50),min(dy,50)
            emu.command('SEND_MOUSE',sx,sy,0);dx-=sx;dy-=sy;time.sleep(.07)
        emu.command('SCREENSHOT',out/('request-%02d-pointer.png'%requests))
        wait(lambda:(out/('request-%02d-pointer-approved'%requests)).exists(),120)
        emu.command('SEND_MOUSE',0,0,1)
        try:time.sleep(.15)
        finally:emu.command('SEND_MOUSE',0,0,0)
        time.sleep(.3)
        emu.command('SCREENSHOT',out/('request-%02d-submitted.png'%requests))
        wait(lambda:'EDITOR FRAME' in log()[offset:] or 'EDITOR EXIT clean' in log()[offset:])
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
    def execute(script):
        reply=subprocess.run([str(infra/'.venv/bin/python'),str(infra/'scripts/mcp-call.py'),
            'amiga_run_script',json.dumps({'script':'Execute '+guest.device+run.name+'/'+script,'timeout':5})],
            capture_output=True,text=True,timeout=20)
        if reply.returncode or '[OK]' not in reply.stdout:raise RuntimeError('Environment setup/restore failed')
    (run/'restore-env').write_text('\n'.join(['CD '+guest.device+run.name,
        'If EXISTS old-recent-env','Copy old-recent-env ENV:PT24G_RECENT_PREFIX','Else',
        'Delete ENV:PT24G_RECENT_PREFIX','EndIf',
        'If EXISTS ENV:PT24G_RECENT_PREFIX','Copy ENV:PT24G_RECENT_PREFIX restored-recent-env','EndIf'])+'\n')
    (run/'setup-env').write_text('\n'.join(['CD '+guest.device+run.name,
        'If EXISTS ENV:PT24G_RECENT_PREFIX','Copy ENV:PT24G_RECENT_PREFIX old-recent-env','EndIf',
        'SetEnv PT24G_RECENT_PREFIX '+guest.device+run.name+'/recent',
        'Copy ENV:PT24G_RECENT_PREFIX active-recent-env'])+'\n')
    try:
        execute('setup-env')
        assert (run/'active-recent-env').read_bytes()==(guest.device+run.name+'/recent').encode(), 'Recent prefix did not match before launch'
        launch.write_text('\n'.join(['FailAt 21','Stack 65536','CD '+guest.device+run.name,
            'PTSourceTest donor.mod >source.log','Echo $RC >source.rc',
            'PT24GEdit input.mod saved.ptg >editor.log','Echo $RC >editor.rc',
            'PT24GEdit saved.ptg reopened.ptg >reopened.log','Echo $RC >reopened.rc',
            'Execute restore-env','Echo done >done'])+'\n')
        launched=True;guest.start()
        frame('status=READY -');assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['source'])
        if args.precision:
            key(0x28,True);request('sample');filename('high.wav');submit_request()
            frame('revision=1 dirty=1 status=SAMPLE UPDATED')
            key(0x13);frame('revision=2 dirty=1');key(0x31,True);frame('revision=1 dirty=1 status=UNDO')
            request('sample');filename('bad.wav');submit_request();frame('revision=1 dirty=1 status=SAMPLE FORMAT OR SLICE')
            key(0x31,True,True);frame('revision=2 dirty=1 status=REDO');key(0x31,True)
            capture('01-stereo24-master.png')
            request('wav');filename('exact.wav');submit_request();frame('WAV EXPORTED AND VERIFIED')
            assert (run/'exact.wav').read_bytes()==high
            key(0x21,True);frame('dirty=0 status=PROJECT SAVED');saved=(run/'saved.ptg').read_bytes()
            record=samples(saved)
            assert record[36:42]==(128).to_bytes(4,'big')+bytes([24,2])
            assert record[64:64+len(pcm)]==b''.join(v.to_bytes(3,'big',signed=True) for v in values)
        else:
            key(0x28,True);request('sample');filename('donor.pp');submit_request();frame('revision=0 dirty=0 status=MOD SOURCE READY')
            capture('01-packed-mod-source.png')
            request('source');offset=key(0x45);frame('SAMPLE FILE REQUEST CANCELLED',offset)
            request('source');filename('donor.mod');submit_request();frame('revision=0 dirty=0 status=MOD SOURCE READY')
            key(0x4e);key(0x4e);key(0x44);frame('revision=0 dirty=0 status=SELECT A NONEMPTY SOURCE')
            key(0x4f);key(0x0c);key(0x0c);capture('02-selected-source-and-destination.png')
            key(0x17);frame('revision=1 dirty=1 status=SOURCE INSTRUMENT IMPORTED')
            key(0x31,True);frame('revision=0 dirty=0 status=UNDO')
            request('source');filename('bad.pp');submit_request();frame('revision=0 dirty=0 status=SAMPLE FORMAT OR SLICE')
            key(0x31,True,True);frame('revision=1 dirty=1 status=REDO');key(0x45);frame('MOD SOURCE CLOSED')
            capture('03-imported-sample.png')
            key(0x21,True);frame('dirty=0 status=PROJECT SAVED');saved=(run/'saved.ptg').read_bytes();dst=samples(saved)
            pos=0;records=[]
            for index in range(31):
                frames=int.from_bytes(dst[pos+36:pos+40],'big');count=int.from_bytes(dst[pos+46:pos+48],'big');size=64+count*4+frames*dst[pos+41]*(dst[pos+40]//8)
                records.append(dst[pos:pos+size]);pos+=size+(-size%4)
            record=records[2];assert record[:32]==b'SOURCE TWO'+bytes(22) and record[40:46]==bytes([8,1,48,253,1,0])
            assert record[48:56]==(8).to_bytes(4,'big')+(24).to_bytes(4,'big') and record[64:]==donor[-32:]
            request('mod');filename('exact.mod');submit_request();frame('MOD EXPORTED AND VERIFIED');assert (run/'exact.mod').read_bytes()==expected_mod
        key(0x45);key(0x45);current='reopened.log';frame('status=READY -')
        if args.precision:
            key(0x28,True);request('wav');filename('copy.wav');submit_request();frame('WAV EXPORTED AND VERIFIED')
            assert (run/'copy.wav').read_bytes()==high
            capture('04-reopened-stereo24.png');key(0x45)
        key(0x21,True);frame('dirty=0 status=PROJECT SAVED');assert (run/'reopened.ptg').read_bytes()==saved
        key(0x45);wait(lambda:(run/'done').exists())
        assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['editor','reopened'])
        previous,restored=run/'old-recent-env',run/'restored-recent-env'
        assert previous.exists()==restored.exists() and (not previous.exists() or previous.read_bytes()==restored.read_bytes())
        assert 'prefix='+guest.device+run.name+'/recent' in (run/'editor.log').read_text()
        audio=audio_off();finished=True;capture('05-normal-exit.png')
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTSourceTest']},
            'isolated_recents_restored':True,'native_source_ownership_fault_tests':True,
            'workflow':'stereo24' if args.precision else 'donor',
            'exact_project_crc_and_reopen_identity':True,'normal_exits':2,'stopped_audio':audio,
            'logs':{n:(run/n).read_text() for n in ['source.log','editor.log','reopened.log']},
            'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        report.update({'exact_stereo24_import_export':True,'master_precision_preserved_through_undo_redo':True,
            'unsupported_import_preserves_redo':True,'reopened_wav_byte_identity':True} if args.precision else {
            'plain_and_pp20_source_preview':True,'browse_and_cancel_preserve_song':True,'empty_source_refused':True,
            'selected_source2_to_destination3_exact':True,'invalid_source_preserves_preview_and_redo':True,
            'whole_song_exact_mod_comparison':True})
        (out/'native-source.json').write_text(json.dumps(report,indent=2)+'\n')
        for n in (['high.wav','exact.wav','copy.wav','saved.ptg','reopened.ptg'] if args.precision else ['donor.mod','donor.pp','saved.ptg','reopened.ptg','exact.mod']):shutil.copyfile(run/n,out/n)
        print('PASS: native '+report['workflow']+' import, exact master PCM, undo/redo, export and reopen')
    finally:
        if not launched:
            execute('restore-env')
        # Normal app cancellation/exit only; never reset/relaunch after failure.
        if launched and not (run/'done').exists():
            for stem in ['editor','reopened']:
                for _ in range(5):
                    if (run/(stem+'.rc')).exists():break
                    path=run/(stem+'.log')
                    if not path.exists() or 'EDITOR FRAME' not in path.read_text():break
                    emu.tap(0x45);time.sleep(1)
            end=time.monotonic()+5
            while not (run/'done').exists() and time.monotonic()<end:time.sleep(.1)
        if (run/'done').exists():
            previous,restored=run/'old-recent-env',run/'restored-recent-env'
            restored_ok=previous.exists()==restored.exists() and (not previous.exists() or previous.read_bytes()==restored.read_bytes())
            state=audio_off()
            (out/'exit.json').write_text(json.dumps({'done':True,'recents_restored':restored_ok,'audio':state},indent=2)+'\n')
        for name in ['source.log','editor.log','reopened.log']:
            if (run/name).exists():shutil.copyfile(run/name,out/name)
        if finished:
            launch.unlink();shutil.rmtree(run)
        (out/'cleanup.json').write_text(json.dumps({'completed_audio_off':finished,'owned_files_cleaned':finished,'run':str(run)},indent=2)+'\n')
        lock.close()
        print('Shared donor evidence:',out)
if __name__=='__main__':main()
