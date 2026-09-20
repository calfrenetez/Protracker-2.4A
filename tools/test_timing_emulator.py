#!/usr/bin/env python3
"""Compare ideal reference clock arithmetic on host and 68k. Reserve Amiberry first."""
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
    run=share/('timing'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/timing-evidence';out.mkdir(exist_ok=True)
    binaries=['PTFrameClockTest','PTTimelineTest']
    for binary in binaries:shutil.copyfile(ROOT/'build/dev'/binary,run/binary)
    expected={}
    with tempfile.TemporaryDirectory() as tmp:
        host=Path(tmp)/'clock'
        subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/frame_clock_test.c','src/core/frame_clock.c','-o',str(host)],cwd=ROOT,check=True)
        for rate in [1,44100,48000,192000]:
            expected[str(rate)]=subprocess.check_output([str(host),str(rate),'10000'],text=True)
    script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            'PTFrameClockTest >clock.log','Echo $RC >clock.rc','PTTimelineTest >timeline.log','Echo $RC >timeline.rc']
    for rate in expected:script += [f'PTFrameClockTest {rate} 10000 >{rate}.log',f'Echo $RC >{rate}.rc']
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
            if (run/'done').exists():break
            time.sleep(.3)
        else:raise RuntimeError('Native timing deadline: '+str(run))
        assert (run/'done').read_text().strip()==run.name
        logs={}
        for name in ['clock','timeline',*expected]:
            log=(run/(name+'.log')).read_text();assert (run/(name+'.rc')).read_text().strip()=='0',(name,log)
            if name in expected:assert log==expected[name],name+' differs from host oracle'
            else:assert ('FRAME CLOCK PASS:' if name=='clock' else 'TIMELINE PASS:') in log
            logs[name]=log
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
                'binaries':{n:digest(run/n) for n in binaries},'logs':logs,'stopped_audio':audio,
                'profile':'IDEAL_BPM_Q32','matched_ticks_per_rate':10000,'scope':'Reference integer frame arithmetic only; CIA timer/audio parity NOT TESTED',
                'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-timing.json').write_text(json.dumps(report,indent=2)+'\n')
        print('PASS: native clock/timeline boundaries and 40,000 per-tick frame hashes match host',flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Timing test released:',run,flush=True)
if __name__=='__main__':main()
