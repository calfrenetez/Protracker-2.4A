#!/usr/bin/env python3
"""Bounded EFx editor workflow on a separately reserved shared030 guest.
Uses an explicitly supplied clean-layout candidate and prepared host references.
"""
import argparse,fcntl,hashlib,json,shutil,sys,time,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def sample_records(blob):
    checked=bytearray(blob);checked[20:24]=bytes(4)
    assert zlib.crc32(checked)==int.from_bytes(blob[20:24],'big')
    pos=32;records=[]
    for _ in range(int.from_bytes(blob[24:28],'big')):
        size=int.from_bytes(blob[pos+8:pos+12],'big')
        if blob[pos:pos+4]==b'SAMP':
            data=blob[pos+12:pos+12+size];at=0
            while at<len(data):
                frames=int.from_bytes(data[at+36:at+40],'big');bits=data[at+40];channels=data[at+41]
                slices=int.from_bytes(data[at+46:at+48],'big');length=64+4*slices+frames*channels*(bits//8)
                records.append(data[at:at+length]);at+=length+(-length%4)
        pos+=12+size+(-size%4)
    return records

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('candidate',type=Path);parser.add_argument('reference',type=Path);args=parser.parse_args()
    sys.path.insert(0,str(INFRA/'scripts'));from shared_guest import Guest
    manifest=json.loads((args.candidate.parent/'editor-build.json').read_text())
    if digest(args.candidate)!=manifest['binary_sha256']:raise RuntimeError('Candidate differs from its clean-layout build manifest')
    out=ROOT/'build/dev'/('invert-ui-'+str(time.time_ns()));out.mkdir()
    result={'passed':False,'scope':'shared030 classic-layout native EFx WAV/stem/bounce UI; no physical acceptance'}
    with (INFRA/'runtime/test.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB);guest=Guest(INFRA,out);run=guest.share/out.name;run.mkdir();finished=False
        fixture=ROOT/'evidence/enhanced-editor/invert-ordering/invert_shared.mod'
        shutil.copyfile(args.candidate,run/'PT24GEdit');shutil.copyfile(fixture,run/'input.mod')
        result.update(candidate_sha256=digest(run/'PT24GEdit'),fixture_sha256=digest(fixture));start=time.monotonic()
        def log():return (run/'editor.log').read_text(errors='replace') if (run/'editor.log').exists() else ''
        def wait(check,seconds=90):
            deadline=min(start+480,time.monotonic()+seconds)
            while time.monotonic()<deadline:
                if check():return
                time.sleep(.1)
            raise RuntimeError('EFx UI deadline; retain owned guest and evidence: '+log()[-600:])
        def frame(text,offset=0):wait(lambda:any('EDITOR FRAME' in line and text in line for line in log()[offset:].splitlines()))
        def key(raw,control=False,shift=False,ack=True):
            offset=len(log())
            if control:guest.command('SEND_KEY',0x63,1)
            if shift:guest.command('SEND_KEY',0x60,1)
            try:guest.tap(raw)
            finally:
                if shift:guest.command('SEND_KEY',0x60,0)
                if control:guest.command('SEND_KEY',0x63,0)
            if ack:wait(lambda:'EDITOR FRAME' in log()[offset:] or 'EDITOR EXIT clean' in log()[offset:])
            return offset
        def filename(text):
            keys=dict(zip('abcdefghijklmnopqrstuvwxyz',[0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31]));keys['.']=0x39
            guest.command('SEND_KEY',0x60,1)
            try:guest.tap(0x4f);guest.tap(0x46)
            finally:guest.command('SEND_KEY',0x60,0)
            for ch in text:guest.tap(keys[ch])
        def request(raw,kind):
            offset=key(raw,ack=False);wait(lambda:'EDITOR REQUEST '+kind in log()[offset:]);time.sleep(.8);return offset
        def capture(name):
            path=out/name;guest.command('SCREENSHOT',path);wait(lambda:path.exists() and path.read_bytes().endswith(b'\0\0\0\0IEND\xaeB`\x82'))
        try:
            # Preserve the global recent-prefix setting and isolate this run's
            # recent files. Restoration executes after normal editor exit only.
            guest.launch.write_text('\n'.join(['FailAt 21','Stack 65536','CD '+guest.device+run.name,
                'If EXISTS ENV:PT24G_RECENT_PREFIX','Copy ENV:PT24G_RECENT_PREFIX previous-prefix','EndIf',
                'SetEnv PT24G_RECENT_PREFIX '+guest.device+run.name+'/recent',
                'PT24GEdit input.mod saved.ptg >editor.log','Echo $RC >editor.rc',
                'If EXISTS previous-prefix','Copy previous-prefix ENV:PT24G_RECENT_PREFIX','Else','Delete ENV:PT24G_RECENT_PREFIX','EndIf',
                'If EXISTS ENV:PT24G_RECENT_PREFIX','Copy ENV:PT24G_RECENT_PREFIX restored-prefix','EndIf',
                'Echo done >'+guest.device+run.name+'/done'])+'\n');guest.start()
            frame('status=READY -');key(0x11,True,True)
            # Two shared-sample audio tracks; EFx clocks remain global.
            key(0x37);key(3);key(0x44);frame('RENDER TRACK MASK SET')
            offset=request(0x11,'render');key(0x45,ack=False);frame('WAV REQUEST CANCELLED',offset)
            offset=request(0x11,'render');filename('render.wav');key(0x44,ack=False);frame('WAV VERIFIED - PROJECT UNCHANGED',offset)
            assert (run/'render.wav').read_bytes()==(args.reference/'render.wav').read_bytes()
            offset=request(0x21,'stems');filename('stems');key(0x44,ack=False);frame('2 STEMS VERIFIED - PROJECT UNCHANGED',offset)
            expected={p.name:p.read_bytes() for p in (args.reference/'stems').iterdir()}
            assert {p.name:p.read_bytes() for p in (run/'stems').iterdir()}==expected
            key(0x14);offset=key(0x16,ack=False);frame('BOUNCED TO SAMPLE 032',offset)
            assert 'EDITOR BOUNCE result=0 detail=0' in log()[offset:]
            key(0x21,True);frame('dirty=0 status=PROJECT SAVED')
            saved=(run/'saved.ptg').read_bytes();records=sample_records(saved);original=sample_records((args.reference/'baseline.ptg').read_bytes())
            assert len(records)==len(original)+1 and records[:-1]==original
            last=records[-1];reference=(args.reference/'track.wav').read_bytes()
            assert last[40:44]==bytes([24,2,64,0]) and int.from_bytes(last[36:40],'big')==(len(reference)-44)//6
            assert last[64:]==b''.join(reference[i:i+3][::-1] for i in range(44,len(reference),3))
            offset=key(0x31,True);frame('revision=0 dirty=1 status=UNDO',offset)
            offset=key(0x31,True,True);frame('revision=1 dirty=0 status=REDO',offset)
            capture('editor-bounce.png');key(0x45);key(0x45);wait(lambda:(run/'done').exists())
            assert (run/'editor.rc').read_text().strip()=='0';finished=True
            assert (run/'input.mod').read_bytes()==fixture.read_bytes() and not list(run.glob('*.pttmp-*')) and not list(run.glob('*.ptstems-*'))
            result.update(passed=True,exact_wav=True,exact_stems=True,source_master_records_unchanged=True,bounced_pcm_exact=True,undo_redo=True,normal_exit=True)
        except Exception as exc:
            result['error']=str(exc)
            # Only normal keys to this owned test app; never reset the guest.
            # Closing a requester/panel and confirming quit lets the launcher
            # restore the exact previous global setting even after an assertion.
            for _ in range(4):
                if (run/'done').exists():break
                guest.tap(0x45);time.sleep(.7)
            if (run/'done').exists() and (run/'editor.rc').read_text().strip()=='0':finished=True
            raise
        finally:
            for name in ['editor.log','editor.rc','saved.ptg','render.wav','previous-prefix']:
                if (run/name).exists():shutil.copyfile(run/name,out/name)
            if (run/'stems').exists():shutil.copytree(run/'stems',out/'stems')
            if finished:
                before=run/'previous-prefix';after=run/'restored-prefix'
                restored=before.exists()==after.exists() and (not before.exists() or before.read_bytes()==after.read_bytes())
                result['recent_prefix_restored']=restored
                finished=restored
            if finished:
                audio=guest.command('GET_AUDIO_STATE');result['cleanup_audio']=audio;finished=all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
            if finished:guest.launch.unlink();shutil.rmtree(run)
            result['run_files_cleaned']=finished
            (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(out,flush=True)
if __name__=='__main__':main()
