#!/usr/bin/env python3
"""Verify pitch-slide rendering against repeated pinned native traces. Reserve Amiberry first."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time
from build_diagnostic import ROOT, digest
from emulator_ipc import Emulator
from make_pitch_fixtures import fixtures
from test_diagnostic_emulator import matching_socket
from test_flow_emulator import decode_trace as decode_flow
import re

def decode_trace(log,budget):
    lines=log.splitlines()
    if not lines or not re.fullmatch(r'FLOW schema=1 bytes=52 count=\d+ reason=(native-stop|tick-budget)',lines[0]):raise ValueError('Invalid pitch header')
    records=[]
    for line in lines[1:-1]:
        if not re.fullmatch(r'T [0-9a-f]{104}',line):raise ValueError('Invalid pitch record')
        records.append(bytes.fromhex(line[2:]))
    control='\n'.join([lines[0].replace('bytes=52','bytes=36'),*['T '+r[:36].hex() for r in records],lines[-1]])
    _,reason=decode_flow(control,budget)
    return b''.join(records),reason

def main():
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('pitch'+str(time.time_ns()));run.mkdir()
    out=ROOT/'build/dev/pitch-evidence'/run.name;out.mkdir(parents=True,exist_ok=False)
    shutil.copyfile(ROOT/'build/dev/PTPitchTraceTest',run/'PTPitchTraceTest')
    shutil.copyfile(ROOT/'build/dev/PTPitchRenderTest',run/'PTPitchRenderTest')
    shutil.copyfile(ROOT/'build/dev/PTPitchTest',run/'PTPitchTest')
    cases=list(fixtures());script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name]
    baseline=ROOT/'evidence/enhanced-editor/dev28/native/speed.mod'
    shutil.copyfile(baseline,run/'baseline.mod')
    script += ['PTPitchTraceTest baseline.mod 160 >baseline.log','Echo $RC >baseline.rc']
    script += ['PTPitchTest >boundaries.log','Echo $RC >boundaries.rc']
    for name,data,metadata in cases:
        (run/(name+'.mod')).write_bytes(data)
        for repeat in range(2):
            stem=name+str(repeat)
            script += [f'PTPitchTraceTest {name}.mod {metadata["max_ticks"]} >{stem}.log',f'Echo $RC >{stem}.rc']
        script += [f'PTPitchTest {name}.mod {name}0.log >{name}-pitch.log',f'Echo $RC >{name}-pitch.rc']
        script += [f'PTPitchRenderTest {name}.mod {name}0.log >{name}-pcm.log',f'Echo $RC >{name}-pcm.rc']
    script+=['Echo '+run.name+' >done'];process=emu=None;start=time.monotonic()
    try:
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as f:
            process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        deadline=start+500;last=-1
        while time.monotonic()<deadline:
            if process.poll() is not None:raise RuntimeError('Emulator exited: '+str(run))
            if not emu:
                matches=matching_socket()
                if len(matches)>1:raise RuntimeError('Ambiguous emulator')
                if matches:emu=Emulator(matches[0])
            complete=len(list(run.glob('*.rc')))
            if complete!=last:
                print(f'Native trace runs complete: {complete}/{2+4*len(cases)}',flush=True);last=complete
            if (run/'done').exists():break
            time.sleep(.3)
        else:raise RuntimeError('Native trace deadline: '+str(run))
        assert (run/'done').read_text().strip()==run.name
        assert (run/'boundaries.rc').read_text().strip()=='0'
        boundary_log=(run/'boundaries.log').read_text();assert 'PITCH boundaries PASS:' in boundary_log
        (out/'boundaries.log').write_text(boundary_log)
        assert (run/'baseline.rc').read_text().strip()=='0'
        baseline_log=(run/'baseline.log').read_text();baseline_trace,_=decode_trace(baseline_log,160)
        assert b''.join(baseline_trace[i:i+36] for i in range(0,len(baseline_trace),52))==(baseline.with_suffix('.trace')).read_bytes()
        (out/'baseline.log').write_text(baseline_log)
        results={}
        for name,data,metadata in cases:
            captures=[]
            for repeat in range(2):
                stem=name+str(repeat);log=(run/(stem+'.log')).read_text()
                assert (run/(stem+'.rc')).read_text().strip()=='0',(stem,log)
                captures.append(decode_trace(log,metadata['max_ticks']))
                (out/(stem+'.log')).write_text(log)
            assert captures[0]==captures[1],name+' repeated trace mismatch'
            pitch_log=(run/(name+'-pitch.log')).read_text()
            assert (run/(name+'-pitch.rc')).read_text().strip()=='0',(name,pitch_log)
            assert 'PITCH native parity PASS:' in pitch_log
            (out/(name+'-pitch.log')).write_text(pitch_log)
            pcm_log=(run/(name+'-pcm.log')).read_text()
            assert (run/(name+'-pcm.rc')).read_text().strip()=='0',(name,pcm_log)
            assert 'PITCH PCM ' in pcm_log and 'PASS:' in pcm_log
            (out/(name+'-pcm.log')).write_text(pcm_log)
            trace,reason=captures[0];(out/(name+'.trace')).write_bytes(trace);(out/(name+'.mod')).write_bytes(data)
            results[name]={**metadata,'native_trace':'CAPTURED TWICE - EXACT MATCH','reason':reason,'ticks':len(trace)//52,
                           'fixture_sha256':hashlib.sha256(data).hexdigest(),'trace_sha256':hashlib.sha256(trace).hexdigest()}
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'schema':1,'record_bytes':52,'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
                'baseline_fixture_sha256':digest(baseline),'baseline_first36':'EXACT DEV28 MATCH','diagnostic_sha256':digest(run/'PTPitchTraceTest'),'pitch_test_sha256':digest(run/'PTPitchTest'),'renderer_test_sha256':digest(run/'PTPitchRenderTest'),'cases':results,'stopped_audio':audio,
                'scope':'Pinned native stored/output periods captured twice; m68k pitch state matches every tick and rendered ramp PCM follows captured periods. Zero period refused before output. Reference rate/timing only; no hardware sound claim.',
                'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-pitch.json').write_text(json.dumps(report,indent=2)+'\n')
        print(f'PASS: {len(cases)} repeated native pitch traces and exact renderer PCM checks; '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Pitch capture released:',run,flush=True)
if __name__=='__main__':main()
