"""Bounded invert-sequence core tests; reserve private emulator first."""
import json,shutil,subprocess,time
from pathlib import Path
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket

def main():
    assert not matching_socket() and not Path('/tmp/amiberry.sock').exists()
    m=json.loads((ROOT/'build/dev/core-build.json').read_text());assert all(digest(ROOT/p)==h for p,h in m['sources'].items())
    name='PTInvertSequenceTest';assert digest(ROOT/'build/dev'/name)==m['binaries'][name]['sha256']
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('invertsequence'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/invert-sequence-evidence'/run.name;out.mkdir(parents=True)
    shutil.copyfile(ROOT/'build/dev'/name,run/name);cases=['fast','slow','disable']
    script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name]
    for case in cases:
        base=ROOT/'evidence/enhanced-editor/dev91/native-reference'/('invert_'+case+'0.log')
        shutil.copyfile(base,run/(case+'.trace'))
        speed=8 if case=='slow' else 15;disable=int(case=='disable')
        script += [f'{name} {case}.trace {speed} {disable} >{case}.log',f'Echo $RC >{case}.rc']
    script+=['Echo '+run.name+' >done'];process=emu=None;start=time.monotonic()
    try:
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        while time.monotonic()-start<180:
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            if emu is None:
                matches=matching_socket();assert len(matches)<=1
                if matches:emu=Emulator(matches[0])
            if (run/'done').exists():break
            time.sleep(.2)
        else:raise RuntimeError('Handoff renderer deadline')
        assert (run/'done').read_text().strip()==run.name
        for case in cases:
            assert (run/(case+'.rc')).read_text().strip()=='0'
            assert (run/(case+'.log')).read_text().strip()=='INVERT sequence native parity PASS'
            shutil.copyfile(run/(case+'.log'),out/(case+'.log'))
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        (out/'native-invert-sequence.json').write_text(json.dumps({'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'binary':m['binaries'][name],'cases':cases,'stopped_audio':audio,'scope':'Core sequence parity with pinned byte snapshots; not renderer or physical acceptance'},indent=2)+'\n');print('PASS: '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            if emu is None:
                matches=matching_socket()
                if len(matches)==1:emu=Emulator(matches[0])
            if emu is None:raise RuntimeError('Retain claim: no guarded socket')
            Emulator(emu.path).command('QUIT');process.wait(timeout=10)
if __name__=='__main__':main()
