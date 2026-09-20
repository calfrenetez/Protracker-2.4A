#!/usr/bin/env python3
"""Capture stored/output volume without changing shipping playback; reserve Amiberry first."""
import hashlib,json,re,shutil,subprocess,time
from pathlib import Path
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket
from test_porta_emulator import decode_trace as decode_pitch
from make_tremolo_fixtures import fixtures

def decode_trace(log,budget):
    lines=log.splitlines()
    assert re.fullmatch(r'FLOW schema=1 bytes=76 count=\d+ reason=(native-stop|tick-budget)',lines[0])
    records=[]
    for line in lines[1:-1]:
        assert re.fullmatch(r'T [0-9a-f]{152}',line);records.append(bytes.fromhex(line[2:]))
    control='\n'.join([lines[0].replace('bytes=76','bytes=52'),*['T '+r[:52].hex() for r in records],lines[-1]])
    _,reason=decode_pitch(control,budget)
    return b''.join(records),reason

def main():
    # Fail closed before any launcher write if inspection is unavailable.
    processes=subprocess.check_output(['ps','ax','-o','pid=,comm='],text=True)
    assert not [line for line in processes.splitlines() if line.lower().endswith('/amiberry')],'Emulator process already owned'
    holders=subprocess.run(['lsof',str(ROOT/'local/baseline.hdf')],capture_output=True,text=True)
    assert holders.returncode==1 and not holders.stdout,'Private HDF owned or inspection failed'
    assert not matching_socket() and not Path('/tmp/amiberry.sock').exists(),'Emulator already owned'
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('tremolo'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/tremolo-evidence'/run.name;out.mkdir(parents=True)
    shutil.copyfile(ROOT/'build/dev/PTVolumeTraceTest',run/'PTVolumeTraceTest')
    cases=list(fixtures());script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name]
    baseline=ROOT/'evidence/enhanced-editor/dev28/native/speed.mod';shutil.copyfile(baseline,run/'baseline.mod')
    script+=['PTVolumeTraceTest baseline.mod 160 >baseline.log','Echo $RC >baseline.rc']
    for name,data,meta in cases:
        (run/(name+'.mod')).write_bytes(data)
        for repeat in range(2):
            stem=name+str(repeat);script += [f'PTVolumeTraceTest {name}.mod {meta["max_ticks"]} >{stem}.log',f'Echo $RC >{stem}.rc']
    script+=['Echo '+run.name+' >done'];process=emu=None;start=time.monotonic()
    try:
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        deadline=start+240;last=-1
        while time.monotonic()<deadline:
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            if not emu:
                matches=matching_socket();assert len(matches)<=1
                if matches:emu=Emulator(matches[0])
            complete=len(list(run.glob('*.rc')))
            for rc in run.glob('*.rc'):
                value=rc.read_text().strip()
                assert not value or value=='0',(rc,value)
            if complete!=last:print(f'Native captures: {complete}/{1+2*len(cases)}',flush=True);last=complete
            if (run/'done').exists() and (run/'done').read_text().strip()==run.name:break
            time.sleep(.3)
        else:raise RuntimeError('Capture deadline')
        assert (run/'done').read_text().strip()==run.name
        assert (run/'baseline.rc').read_text().strip()=='0'
        log=(run/'baseline.log').read_text();trace,_=decode_trace(log,160)
        old,_=decode_pitch((ROOT/'evidence/enhanced-editor/dev38/native/baseline.log').read_text(),160)
        assert b''.join(trace[i:i+52] for i in range(0,len(trace),76))==old
        (out/'baseline.log').write_text(log);results={}
        for name,data,meta in cases:
            captures=[]
            for repeat in range(2):
                stem=name+str(repeat);log=(run/(stem+'.log')).read_text();assert (run/(stem+'.rc')).read_text().strip()=='0',(stem,log)
                captures.append(decode_trace(log,meta['max_ticks']));(out/(stem+'.log')).write_text(log)
            assert captures[0]==captures[1],name
            trace,reason=captures[0];(out/(name+'.mod')).write_bytes(data);(out/(name+'.trace')).write_bytes(trace)
            results[name]={**meta,'native_trace':'CAPTURED TWICE - EXACT MATCH','reason':reason,'ticks':len(trace)//76,'fixture_sha256':hashlib.sha256(data).hexdigest(),'trace_sha256':hashlib.sha256(trace).hexdigest()}
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'schema':1,'record_bytes':76,'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'diagnostic_sha256':digest(run/'PTVolumeTraceTest'),'baseline_first52':'EXACT DEV38 MATCH','cases':results,'stopped_audio':audio,'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']},'scope':'7xy/E7x stored/output volume and waveform state only. Renderer enablement and physical acceptance remain open.'}
        (out/'native-tremolo.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS: '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Volume capture released:',run,flush=True)
if __name__=='__main__':main()
