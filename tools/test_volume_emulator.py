#!/usr/bin/env python3
"""Verify volume-effect rendering against repeated pinned native traces. Reserve Amiberry first."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time
from build_diagnostic import ROOT, digest
from emulator_ipc import Emulator
from make_volume_fixtures import fixtures
from test_diagnostic_emulator import matching_socket
from test_flow_emulator import decode_trace

def main():
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('volume'+str(time.time_ns()));run.mkdir()
    out=ROOT/'build/dev/volume-evidence'/run.name;out.mkdir(parents=True,exist_ok=False)
    shutil.copyfile(ROOT/'build/dev/PTFlowTraceTest',run/'PTFlowTraceTest')
    shutil.copyfile(ROOT/'build/dev/PTVolumeRenderTest',run/'PTVolumeRenderTest')
    cases=list(fixtures());script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name]
    script += ['PTVolumeRenderTest >boundaries.log','Echo $RC >boundaries.rc']
    for name,data,metadata in cases:
        (run/(name+'.mod')).write_bytes(data)
        for repeat in range(2):
            stem=name+str(repeat)
            script += [f'PTFlowTraceTest {name}.mod {metadata["max_ticks"]} >{stem}.log',f'Echo $RC >{stem}.rc']
        script += [f'PTVolumeRenderTest {name}.mod {name}0.log >{name}-pcm.log',f'Echo $RC >{name}-pcm.rc']
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
                print(f'Native trace runs complete: {complete}/{1+3*len(cases)}',flush=True);last=complete
            if (run/'done').exists():break
            time.sleep(.3)
        else:raise RuntimeError('Native trace deadline: '+str(run))
        assert (run/'done').read_text().strip()==run.name
        assert (run/'boundaries.rc').read_text().strip()=='0'
        boundary_log=(run/'boundaries.log').read_text();assert 'VOLUME boundaries PASS:' in boundary_log
        (out/'boundaries.log').write_text(boundary_log)
        results={}
        for name,data,metadata in cases:
            captures=[]
            for repeat in range(2):
                stem=name+str(repeat);log=(run/(stem+'.log')).read_text()
                assert (run/(stem+'.rc')).read_text().strip()=='0',(stem,log)
                captures.append(decode_trace(log,metadata['max_ticks']))
                (out/(stem+'.log')).write_text(log)
            assert captures[0]==captures[1],name+' repeated trace mismatch'
            pcm_log=(run/(name+'-pcm.log')).read_text()
            assert (run/(name+'-pcm.rc')).read_text().strip()=='0',(name,pcm_log)
            assert 'VOLUME native-trace PCM PASS:' in pcm_log
            (out/(name+'-pcm.log')).write_text(pcm_log)
            trace,reason=captures[0];(out/(name+'.trace')).write_bytes(trace);(out/(name+'.mod')).write_bytes(data)
            results[name]={**metadata,'native_trace':'CAPTURED TWICE - EXACT MATCH','reason':reason,'ticks':len(trace)//36,
                           'fixture_sha256':hashlib.sha256(data).hexdigest(),'trace_sha256':hashlib.sha256(trace).hexdigest()}
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'schema':1,'record_bytes':36,'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
                'diagnostic_sha256':digest(run/'PTFlowTraceTest'),'renderer_test_sha256':digest(run/'PTVolumeRenderTest'),'cases':results,'stopped_audio':audio,
                'scope':'Pinned native volume traces captured twice; every constant-source PCM frame checked against raw native volume by the m68k reference renderer; silent phase continuity on 16 tracks. No hardware or analogue sound validation.',
                'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-volume.json').write_text(json.dumps(report,indent=2)+'\n')
        print(f'PASS: {len(cases)} repeated native volume traces and exact renderer PCM checks; '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Volume capture released:',run,flush=True)
if __name__=='__main__':main()
