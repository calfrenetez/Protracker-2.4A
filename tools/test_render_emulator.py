#!/usr/bin/env python3
"""Verify bounded reference rendering and DOS WAV publication. Reserve Amiberry first."""
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
    run=share/('render'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/render-evidence'/run.name;out.mkdir(parents=True,exist_ok=False)
    binaries=['PTRenderTest','PTRenderFileTest','PT24GRender']
    for binary in binaries:shutil.copyfile(ROOT/'build/dev'/binary,run/binary)
    fixture=ROOT/'evidence/enhanced-editor/dev28/native/speed.mod';shutil.copyfile(fixture,run/'input.mod')
    unsupported=bytearray(fixture.read_bytes());unsupported[1086]=(unsupported[1086]&0xf0)|9;(run/'unsupported.mod').write_bytes(unsupported)
    subprocess.run(['make','renderer'],cwd=ROOT,check=True)
    oracle=out/'host.wav'
    host_log=subprocess.check_output([str(ROOT/'build/host/PT24GRender'),str(fixture),str(oracle),'--rate','44100','--bits','16','--gain','65536'],text=True)
    (out/'host.log').write_text(host_log)
    cases=[('core','PTRenderTest',0),('files','PTRenderFileTest .',0),
           ('render','PT24GRender input.mod native.wav --rate 44100 --bits 16 --gain 65536',0),
           ('existing','PT24GRender input.mod native.wav --rate 44100 --bits 16 --gain 65536',20),
           ('pattern','PT24GRender input.mod pattern.wav --pattern 0 --rate 44100 --bits 16 --gain 65536',0),
           ('refused','PT24GRender unsupported.mod refused.wav',20),('invalid','PT24GRender input.mod invalid.wav --bits 8',20)]
    script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name]
    for name,command,_ in cases:script += [command+' >'+name+'.log','Echo $RC >'+name+'.rc']
    script+=['Echo '+run.name+' >done'];process=emu=None;start=time.monotonic()
    try:
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as f:
            process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        deadline=start+300;last=-1
        while time.monotonic()<deadline:
            if process.poll() is not None:raise RuntimeError('Emulator exited: '+str(run))
            if not emu:
                matches=matching_socket()
                if len(matches)>1:raise RuntimeError('Ambiguous emulator')
                if matches:emu=Emulator(matches[0])
            count=len(list(run.glob('*.rc')))
            if count!=last:print(f'Native render executions complete: {count}/{len(cases)}',flush=True);last=count
            if (run/'done').exists():break
            time.sleep(.3)
        else:raise RuntimeError('Native render deadline: '+str(run))
        assert (run/'done').read_text().strip()==run.name
        logs={}
        for name,_,expected in cases:
            log=(run/(name+'.log')).read_text();assert (run/(name+'.rc')).read_text().strip()==str(expected),(name,log)
            logs[name]=log
        assert 'RENDER PASS:' in logs['core'] and 'RENDER FILE PASS:' in logs['files']
        assert logs['render']==host_log and logs['pattern']==host_log
        assert (run/'native.wav').read_bytes()==oracle.read_bytes()==(run/'pattern.wav').read_bytes()
        assert (run/'race.wav').read_bytes()==b'PRESERVE'
        for name in ['cancel.wav','corrupt.wav','verifycancel.wav','unsupported.wav','refused.wav','invalid.wav']:assert not (run/name).exists(),name
        assert not list(run.glob('*.pttmp-*'))
        # Independently inspect every true24 stereo byte from the DOS save test.
        saved=(run/'saved.wav').read_bytes();assert len(saved)==5804 and saved[:4]==b'RIFF'
        assert saved[44:]==((0x123456).to_bytes(3,'little')+((-0x345678)&0xffffff).to_bytes(3,'little'))*960
        for name in ['native.wav','pattern.wav','saved.wav']:shutil.copyfile(run/name,out/name)
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
                'binaries':{n:digest(run/n) for n in binaries},'fixture_sha256':digest(fixture),'logs':logs,'stopped_audio':audio,
                'wave_files':{n:{'sha256':digest(out/n),'bytes':(out/n).stat().st_size} for n in ['host.wav','native.wav','pattern.wav','saved.wav']},
                'scope':'Bounded supported-subset reference WAV renderer and no-replace DOS publication; full effects/CIA/hardware audio NOT TESTED',
                'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-render.json').write_text(json.dumps(report,indent=2)+'\n')
        print('PASS: native reference PCM, exact host WAV parity, verified DOS saves and failure preservation; '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Render test released:',run,flush=True)
if __name__=='__main__':main()
