#!/usr/bin/env python3
"""Host/native stem parity and directory publication. Reserve Amiberry first."""
import json,shutil,subprocess,time
from pathlib import Path
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket

def main():
    processes=subprocess.check_output(['ps','ax','-o','pid=,comm='],text=True)
    assert not any(line.lower().endswith('/amiberry') for line in processes.splitlines())
    holders=subprocess.run(['lsof',str(ROOT/'local/baseline.hdf')],capture_output=True,text=True)
    assert holders.returncode==1 and not holders.stdout
    assert not matching_socket() and not Path('/tmp/amiberry.sock').exists()
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('stems'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/stems-evidence'/run.name;out.mkdir(parents=True)
    binaries=['PTStemsTest','PTStemFileTest','PT24GRender']
    for n in binaries:shutil.copyfile(ROOT/'build/dev'/n,run/n)
    fixture=ROOT/'evidence/enhanced-editor/dev48/native/fine_00.mod';shutil.copyfile(fixture,run/'input.mod')
    data=bytearray(fixture.read_bytes());data[1086]=(data[1086]&240)|14;data[1087]=0xf1;(run/'bad.mod').write_bytes(data)
    host_log=subprocess.check_output([str(ROOT/'build/host/PT24GRender'),str(fixture),str(out/'host'),'--stems','--tracks','3'],text=True)
    (out/'host.log').write_text(host_log)
    cases=[('plan','PTStemsTest',0),('files','PTStemFileTest PTDEV:'+run.name,0),
           ('cli','PT24GRender input.mod exported --stems --tracks 3',0),
           ('existing','PT24GRender input.mod exported --stems --tracks 3',20),
           ('refused','PT24GRender bad.mod refused --stems',20)]
    script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name]
    for name,cmd,_ in cases:script += [cmd+' >'+name+'.log','Echo $RC >'+name+'.rc']
    script+=['Echo '+run.name+' >done'];process=emu=None;start=time.monotonic()
    try:
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as log:
            process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        deadline=start+600;last=-1
        while time.monotonic()<deadline:
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            if not emu:
                sockets=matching_socket();assert len(sockets)<=1
                if sockets:emu=Emulator(sockets[0])
            count=0
            for log in run.glob('*.log'):
                if log.name!='emulator.log':assert 'Program aborted' not in log.read_text(errors='replace'),log.read_text(errors='replace')
            for name,_,expected in cases:
                rc=run/(name+'.rc')
                if rc.exists() and rc.read_text().strip():
                    assert rc.read_text().strip()==str(expected),(name,(run/(name+'.log')).read_text());count+=1
            if count!=last:print(f'Native stems checks {count}/{len(cases)}',flush=True);last=count
            if (run/'done').exists() and (run/'done').read_text().strip()==run.name:break
            time.sleep(.3)
        else:raise RuntimeError('Stem test deadline')
        assert count==len(cases)
        logs={name:(run/(name+'.log')).read_text() for name,_,_ in cases}
        assert 'STEMS plan PASS:' in logs['plan'] and 'STEMS files PASS:' in logs['files']
        assert logs['cli']==host_log
        for p in (out/'host').iterdir():assert p.read_bytes()==(run/'exported'/p.name).read_bytes()
        assert sorted(p.name for p in (run/'exported').iterdir())==['track-01.wav','track-02.wav']
        assert not (run/'cancel').exists() and not (run/'refused').exists() and list((run/'race').iterdir())==[]
        assert not list(run.glob('*.ptstems-*')) and not list(run.rglob('*.pttmp-*'))
        shutil.copytree(run/'exported',out/'native');shutil.copytree(run/'batch',out/'grouped')
        for n,log in logs.items():(out/(n+'.log')).write_text(log)
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'checks':len(cases),
                'binaries':{n:digest(run/n) for n in binaries},'fixture_sha256':digest(fixture),'stopped_audio':audio,
                'scope':'Exact native/host reference WAV parity, grouped export equivalence, global timing, no-replace directory publication and cancellation cleanup. No physical or analogue sound claim.'}
        (out/'native-stems.json').write_text(json.dumps(report,indent=2)+'\n');print('STEMS PASS: '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Stem emulator released:',run,flush=True)
if __name__=='__main__':main()
