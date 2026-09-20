#!/usr/bin/env python3
"""Check immutable high-resolution voice traversal and mixing on 68k. Reserve Amiberry first."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket

def main():
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('segment'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/segment-evidence';out.mkdir(exist_ok=True)
    binaries=['PTVoiceSegmentTest','PTVoiceTest']
    for binary in binaries:shutil.copyfile(ROOT/'build/dev'/binary,run/binary)
    with tempfile.TemporaryDirectory() as tmp:
        host=Path(tmp)/'voice'
        subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/voice_segment_test.c','src/core/voice.c','src/core/pcm.c','-o',str(host)],cwd=ROOT,check=True)
        expected=subprocess.check_output([str(host)],text=True)
    script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTVoiceSegmentTest >voice.log','Echo $RC >voice.rc','PTVoiceTest >legacy.log','Echo $RC >legacy.rc']
    script+=['Echo '+run.name+' >done'];process=emu=None;start=time.monotonic()
    try:
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as f:
            process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        deadline=start+180
        while time.monotonic()<deadline:
            if process.poll() is not None:raise RuntimeError('Emulator exited: '+str(run))
            if not emu:
                matches=matching_socket()
                if len(matches)>1:raise RuntimeError('Ambiguous emulator')
                if matches:emu=Emulator(matches[0])
            if (run/'done').exists() and (run/'done').read_text().strip()==run.name:break
            time.sleep(.3)
        else:raise RuntimeError('Native voice deadline: '+str(run))
        assert (run/'done').read_text().strip()==run.name
        log=(run/'voice.log').read_text();assert (run/'voice.rc').read_text().strip()=='0',log
        assert log==expected,'Native voice test differs from host oracle'
        assert 'VOICE segment PASS:' in log
        legacy=(run/'legacy.log').read_text();assert (run/'legacy.rc').read_text().strip()=='0' and 'hash=990c42f0 clips=40' in legacy
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
                'binaries':{n:digest(run/n) for n in binaries},'log':log,'legacy_log':legacy,'stopped_audio':audio,
                'scope':'Reference voice/mix arithmetic only; hardware audio and full tracker-effect parity NOT TESTED',
                'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-segment.json').write_text(json.dumps(report,indent=2)+'\n')
        print('PASS: native initial-segment/loop handoffs and trajectories match host',flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Voice test released:',run,flush=True)
if __name__=='__main__':main()
