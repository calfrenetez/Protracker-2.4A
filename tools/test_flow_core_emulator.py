#!/usr/bin/env python3
"""Run the portable flow comparisons on 68k. Reserve Amiberry first."""
import json
from pathlib import Path
import shutil
import subprocess
import time
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket

def main():
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('flowcore'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/flow-core-evidence';out.mkdir(exist_ok=True)
    source=ROOT/'evidence/enhanced-editor/dev28/native';cases=json.loads((source/'native-flow.json').read_text())['cases']
    shutil.copyfile(ROOT/'build/dev/PTFlowCoreTest',run/'PTFlowCoreTest')
    script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,'PTFlowCoreTest >bounds.log','Echo $RC >bounds.rc']
    for name,case in cases.items():
        for suffix in ['.mod','.trace']:shutil.copyfile(source/(name+suffix),run/(name+suffix))
        script += [f'PTFlowCoreTest {name}.mod {name}.trace {case["max_ticks"]} >{name}.log',f'Echo $RC >{name}.rc']
    script+=['Echo '+run.name+' >done'];process=emu=None;start=time.monotonic()
    try:
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as f:
            process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        deadline=start+150
        while time.monotonic()<deadline:
            if process.poll() is not None:raise RuntimeError('Emulator exited: '+str(run))
            if not emu:
                matches=matching_socket()
                if len(matches)>1:raise RuntimeError('Ambiguous emulator')
                if matches:emu=Emulator(matches[0])
            if (run/'done').exists():break
            time.sleep(.3)
        else:raise RuntimeError('Native core flow deadline: '+str(run))
        assert (run/'done').read_text().strip()==run.name
        logs={}
        for name in ['bounds',*cases]:
            log=(run/(name+'.log')).read_text();assert (run/(name+'.rc')).read_text().strip()=='0',(name,log)
            if name=='bounds':assert 'FLOW boundaries PASS:' in log
            else:assert f'FLOW parity PASS ticks={cases[name]["ticks"]} reason={cases[name]["reason"]}' in log
            logs[name]=log
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'diagnostic_sha256':digest(run/'PTFlowCoreTest'),
                'native_reference_sha256':digest(source/'native-flow.json'),'logs':logs,'stopped_audio':audio,
                'scope':'Portable 68k row/tick flow parity; audio mixing and physical hardware NOT TESTED',
                'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-flow-core.json').write_text(json.dumps(report,indent=2)+'\n')
        print('PASS: portable 68k boundaries and all 16 captured native flow fixtures',flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Flow core test released:',run,flush=True)
if __name__=='__main__':main()
