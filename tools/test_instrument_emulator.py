#!/usr/bin/env python3
"""Capture instrument-only semantics without changing shipping replay; reserve Amiberry first."""
import hashlib,json,re,shutil,subprocess,time,sys
from pathlib import Path
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket
from test_porta_emulator import decode_trace as decode_pitch
from make_instrument_fixtures import fixtures

def decode_trace(log,budget):
    lines=log.splitlines()
    assert re.fullmatch(r'FLOW schema=1 bytes=140 count=\d+ reason=(native-stop|tick-budget)',lines[0])
    records=[]
    for line in lines[1:-1]:
        assert re.fullmatch(r'T [0-9a-f]{280}',line);records.append(bytes.fromhex(line[2:]))
    control='\n'.join([lines[0].replace('bytes=140','bytes=52'),*['T '+r[:52].hex() for r in records],lines[-1]])
    _,reason=decode_pitch(control,budget)
    return b''.join(records),reason

def main():
    processes=subprocess.check_output(['ps','ax','-o','pid=,comm='],text=True)
    assert not any(line.lower().endswith('/amiberry') for line in processes.splitlines())
    holders=subprocess.run(['lsof',str(ROOT/'local/baseline.hdf')],capture_output=True,text=True)
    assert holders.returncode==1 and not holders.stdout
    manifest=json.loads((ROOT/'build/dev/core-build.json').read_text())
    assert all(digest(ROOT/p)==h for p,h in manifest['sources'].items())
    assert not matching_socket() and not Path('/tmp/amiberry.sock').exists(),'Emulator already owned'
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    assert digest(launch)=='9c801b94c85b06cd124fe2d611f185bfe135c8c7442f8263a761c79102f8ddf3'
    run=share/('instrument'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/instrument-evidence'/run.name;out.mkdir(parents=True)
    assert digest(ROOT/'build/dev/PTSampleTraceTest')==manifest['binaries']['PTSampleTraceTest']['sha256']
    shutil.copyfile(ROOT/'build/dev/PTSampleTraceTest',run/'PTSampleTraceTest')
    source=fixtures
    if '--handoff' in sys.argv:
        from make_handoff_fixtures import fixtures as source
    if '--volume-handoff' in sys.argv:
        from make_handoff_fixtures import volume_fixtures as source
    if '--slide-handoff' in sys.argv:
        from make_handoff_fixtures import slide_fixtures as source
    if '--cut-handoff' in sys.argv:
        from make_handoff_fixtures import cut_fixtures as source
    if '--tone-note-handoff' in sys.argv:
        from make_handoff_fixtures import tone_note_fixtures as source
    if '--tone-handoff' in sys.argv:
        from make_handoff_fixtures import tone_fixtures as source
    if '--modulation-handoff' in sys.argv:
        from make_handoff_fixtures import modulation_fixtures as source
    if '--pitch-handoff' in sys.argv:
        from make_handoff_fixtures import pitch_fixtures as source
    if '--silent-handoff' in sys.argv:
        from make_handoff_fixtures import silent_tail_fixtures as source
    cases=list(source());script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name]
    baseline=ROOT/'evidence/enhanced-editor/dev28/native/speed.mod';shutil.copyfile(baseline,run/'baseline.mod')
    script+=['PTSampleTraceTest baseline.mod 160 >baseline.log','Echo $RC >baseline.rc']
    for name,data,meta in cases:
        (run/(name+'.mod')).write_bytes(data)
        for repeat in range(2):
            stem=name+str(repeat);script += [f'PTSampleTraceTest {name}.mod {meta["max_ticks"]} >{stem}.log',f'Echo $RC >{stem}.rc']
    script+=['Echo '+run.name+' >done'];process=emu=None;start=time.monotonic()
    try:
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        deadline=start+360;last=-1
        while time.monotonic()<deadline:
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            if not emu:
                matches=matching_socket();assert len(matches)<=1
                if matches:emu=Emulator(matches[0])
            complete=len(list(run.glob('*.rc')))
            if complete!=last:print(f'Native captures: {complete}/{1+2*len(cases)}',flush=True);last=complete
            if (run/'done').exists() and (run/'done').read_text().strip()==run.name:break
            time.sleep(.3)
        else:raise RuntimeError('Capture deadline')
        assert (run/'done').read_text().strip()==run.name
        assert (run/'baseline.rc').read_text().strip()=='0'
        log=(run/'baseline.log').read_text();trace,_=decode_trace(log,160)
        old,_=decode_pitch((ROOT/'evidence/enhanced-editor/dev38/native/baseline.log').read_text(),160)
        assert b''.join(trace[i:i+52] for i in range(0,len(trace),140))==old
        (out/'baseline.log').write_text(log);results={}
        for name,data,meta in cases:
            captures=[]
            for repeat in range(2):
                stem=name+str(repeat);log=(run/(stem+'.log')).read_text();assert (run/(stem+'.rc')).read_text().strip()=='0',(stem,log)
                captures.append(decode_trace(log,meta['max_ticks']));(out/(stem+'.log')).write_text(log)
            assert captures[0]==captures[1],name
            trace,reason=captures[0];(out/(name+'.mod')).write_bytes(data);(out/(name+'.trace')).write_bytes(trace)
            results[name]={**meta,'native_trace':'CAPTURED TWICE - EXACT MATCH','reason':reason,'ticks':len(trace)//140,'fixture_sha256':hashlib.sha256(data).hexdigest(),'trace_sha256':hashlib.sha256(trace).hexdigest()}
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'schema':1,'record_bytes':140,'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'diagnostic_sha256':digest(run/'PTSampleTraceTest'),'baseline_first52':'EXACT DEV38 MATCH','cases':results,'stopped_audio':audio,'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']},'scope':'Instrument-only stored/output period, volume, sample ranges and DMA trigger snapshots. Renderer acceptance requires separate parity checks; no physical acceptance.'}
        (out/'native-instrument.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS: '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            if emu is None:
                matches=matching_socket()
                if len(matches)==1:emu=Emulator(matches[0])
            if emu is None:raise RuntimeError('Retain claim: guarded socket unavailable')
            Emulator(emu.path).command('QUIT');process.wait(timeout=10)
        print('Instrument capture released:',run,flush=True)
if __name__=='__main__':main()
