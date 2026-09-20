#!/usr/bin/env python3
"""Verify instrument-only renderer against retained native traces; reserve first."""
import json
from pathlib import Path
import shutil,subprocess,time
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket
from make_instrument_fixtures import fixtures


def main():
    processes=subprocess.check_output(['ps','ax','-o','pid=,comm='],text=True)
    assert not any(s.lower().endswith('/amiberry') for s in processes.splitlines())
    holders=subprocess.run(['lsof',str(ROOT/'local/baseline.hdf')],capture_output=True,text=True)
    assert holders.returncode==1 and not holders.stdout
    assert not matching_socket() and not Path('/tmp/amiberry.sock').exists()
    manifest=json.loads((ROOT/'build/dev/core-build.json').read_text())
    assert all(digest(ROOT/p)==h for p,h in manifest['sources'].items())
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    assert digest(launch)=='9c801b94c85b06cd124fe2d611f185bfe135c8c7442f8263a761c79102f8ddf3'
    run=share/('instrumentrender'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/instrument-render-evidence'/run.name;out.mkdir(parents=True)
    names=['PTInstrumentRenderTest','PTOffsetRenderTest','PT24GRender']
    for n in names:shutil.copyfile(ROOT/'build/dev'/n,run/n)
    evidence=ROOT/'evidence/enhanced-editor/dev54/native';cases=[('boundary','PTInstrumentRenderTest',0)]
    subprocess.run(['make','renderer'],cwd=ROOT,check=True)
    expected={}
    for name,data,_ in fixtures():
        assert (evidence/(name+'.mod')).read_bytes()==data
        shutil.copyfile(evidence/(name+'.mod'),run/(name+'.mod'));shutil.copyfile(evidence/(name+'0.log'),run/(name+'.trace.log'))
        cases.append((name,'PTOffsetRenderTest '+name+'.mod '+name+'.trace.log 0',0))
        if name in ('instrument_preload','instrument_retrig'):
            opts=['--rate','44100','--bits','16'] if name.endswith('preload') else ['--rate','48000','--bits','24']
            target=out/(name+'.wav');expected[name]=subprocess.check_output([str(ROOT/'build/host/PT24GRender'),str(run/(name+'.mod')),str(target),*opts],text=True)
            cases.append((name+'_wav','PT24GRender '+name+'.mod '+name+'.wav '+' '.join(opts),0))
    # Different active instrument remains a valid MOD but unsupported handoff.
    data=bytearray((run/'instrument_reload.mod').read_bytes());data[50:80]=data[20:50];data.extend(data[2108:]);data[1084+16+2]=0x20
    (run/'handoff.mod').write_bytes(data);cases.append(('handoff','PT24GRender handoff.mod refused.wav',20))
    script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name]
    for name,command,_ in cases:script.extend([command+' >'+name+'.log','Echo $RC >'+name+'.rc'])
    script.append('Echo '+run.name+' >done');process=emu=None;start=time.monotonic()
    try:
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=f,start_new_session=True)
        deadline=start+360;last=-1
        while time.monotonic()<deadline:
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            if not emu:
                matches=matching_socket();assert len(matches)<=1
                if matches:emu=Emulator(matches[0])
            count=len(list(run.glob('*.rc')))
            if count!=last:print('Native instrument render executions: %u/%u'%(count,len(cases)),flush=True);last=count
            if (run/'done').exists():break
            time.sleep(.3)
        else:raise RuntimeError('Instrument render deadline')
        assert (run/'done').read_text().strip()==run.name
        logs={}
        for name,_,code in cases:
            logs[name]=(run/(name+'.log')).read_text();assert (run/(name+'.rc')).read_text().strip()==str(code),(name,logs[name])
        for name,host in expected.items():
            assert logs[name+'_wav']==host and (run/(name+'.wav')).read_bytes()==(out/(name+'.wav')).read_bytes()
        assert not (run/'refused.wav').exists() and not list(run.glob('*.pttmp-*'))
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'executions':len(cases),'binaries':{n:digest(run/n) for n in names},'logs':logs,'native_trace_driven_pcm':True,'exact_host_cli_wavs':True,'cross_sample_handoff_refused_without_output':True,'stopped_audio':audio,'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-instrument-render.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS: '+str(out),flush=True)
    finally:
        for source in run.glob('*.log'):
            if source.name!='emulator.log':shutil.copyfile(source,out/source.name)
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Instrument renderer released:',run,flush=True)
if __name__=='__main__':main()
