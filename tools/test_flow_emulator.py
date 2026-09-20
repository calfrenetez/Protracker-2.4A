#!/usr/bin/env python3
"""Capture repeated diagnostic tick traces. Reserve Amiberry before running."""
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import time
from build_diagnostic import ROOT, digest
from emulator_ipc import Emulator
from make_replay_flow_fixtures import fixtures
from test_diagnostic_emulator import matching_socket

def decode_trace(log, budget):
    lines=log.splitlines()
    if not lines or lines[-1]!='FLOW PASS dma=0':raise ValueError('Incomplete native trace')
    header=re.fullmatch(r'FLOW schema=1 bytes=36 count=(\d+) reason=(native-stop|tick-budget)',lines[0])
    if not header:raise ValueError('Invalid trace header')
    count=int(header[1]);reason=header[2]
    if not 0<count<=budget or len(lines)!=count+2:raise ValueError('Invalid trace count')
    records=[]
    for index,line in enumerate(lines[1:-1]):
        if not re.fullmatch(r'T [0-9a-f]{72}',line):raise ValueError('Invalid trace record')
        record=bytes.fromhex(line[2:]);tick=int.from_bytes(record[:4],'big')
        if tick!=index+1:raise ValueError('Nonsequential tick')
        records.append(record)
    if reason=='tick-budget' and (count!=budget or not records[-1][14]):raise ValueError('Invalid budget stop')
    if reason=='native-stop' and records[-1][14]:raise ValueError('Native stop still enabled')
    if any(not r[14] for r in records[:-1]):raise ValueError('Disabled tick before last record')
    return b''.join(records),reason

def main():
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('flow'+str(time.time_ns()));run.mkdir()
    out=ROOT/'build/dev/flow-evidence'/run.name;out.mkdir(parents=True,exist_ok=False)
    shutil.copyfile(ROOT/'build/dev/PTFlowTraceTest',run/'PTFlowTraceTest')
    cases=list(fixtures());script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name]
    for name,data,metadata in cases:
        (run/(name+'.mod')).write_bytes(data)
        for repeat in range(2):
            stem=name+str(repeat)
            script += [f'PTFlowTraceTest {name}.mod {metadata["max_ticks"]} >{stem}.log',f'Echo $RC >{stem}.rc']
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
                print(f'Native trace runs complete: {complete}/32',flush=True);last=complete
            if (run/'done').exists():break
            time.sleep(.3)
        else:raise RuntimeError('Native trace deadline: '+str(run))
        assert (run/'done').read_text().strip()==run.name
        results={}
        for name,data,metadata in cases:
            captures=[]
            for repeat in range(2):
                stem=name+str(repeat);log=(run/(stem+'.log')).read_text()
                assert (run/(stem+'.rc')).read_text().strip()=='0',(stem,log)
                captures.append(decode_trace(log,metadata['max_ticks']))
                (out/(stem+'.log')).write_text(log)
            assert captures[0]==captures[1],name+' repeated trace mismatch'
            trace,reason=captures[0];(out/(name+'.trace')).write_bytes(trace);(out/(name+'.mod')).write_bytes(data)
            results[name]={**metadata,'native_trace':'CAPTURED TWICE - EXACT MATCH','reason':reason,'ticks':len(trace)//36,
                           'fixture_sha256':hashlib.sha256(data).hexdigest(),'trace_sha256':hashlib.sha256(trace).hexdigest()}
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'schema':1,'record_bytes':36,'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
                'diagnostic_sha256':digest(run/'PTFlowTraceTest'),'cases':results,'stopped_audio':audio,
                'scope':'Pinned native flow traces only; portable sequencer/audio parity NOT RUN',
                'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-flow.json').write_text(json.dumps(report,indent=2)+'\n')
        print('PASS: 16 fixtures captured twice with identical native records; '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Flow capture released:',run,flush=True)
if __name__=='__main__':main()
