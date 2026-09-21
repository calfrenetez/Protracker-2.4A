"""Bounded reference-derived PCM tests; reserve private emulator first."""
import json,shutil,subprocess,time,sys
from pathlib import Path
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket

def main():
    assert not matching_socket() and not Path('/tmp/amiberry.sock').exists()
    m=json.loads((ROOT/'build/dev/core-build.json').read_text());assert all(digest(ROOT/p)==h for p,h in m['sources'].items())
    name='PTHandoffTest';assert digest(ROOT/'build/dev'/name)==m['binaries'][name]['sha256']
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('handoffrender'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/handoff-render-evidence'/run.name;out.mkdir(parents=True)
    shutil.copyfile(ROOT/'build/dev'/name,run/name);cases=['retrigzero','retriglater'] if '--repeat-track' in sys.argv else ['retrig2','retrig3'] if '--retrigger' in sys.argv else ['jump','break'] if '--jump' in sys.argv else ['speed','tempo'] if '--tempo' in sys.argv else ['delay','loopflow'] if '--flow' in sys.argv else ['tuneplus','tuneminus','glisson','glissoff'] if '--tuning' in sys.argv else ['vibramp','vibsquare','tremramp','tremsquare'] if '--waveform' in sys.argv else ['noteup','notedown','notevolup','notevoldown'] if '--tone-note' in sys.argv else ['toneup','tonedown','tonevolup','tonevoldown'] if '--tone' in sys.argv else ['arp','vib','trem','vibvol'] if '--modulation' in sys.argv else ['pitchup','pitchdown','finepup','finepdown'] if '--pitch' in sys.argv else ['silent'] if '--silent' in sys.argv else ['cut0','cut3'] if '--cuts' in sys.argv else ['up','down','fineup','finedown'] if '--slides' in sys.argv else ['volume','clamp'] if '--volume' in sys.argv else ['loop','return','noloop','ed'];script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name]
    for case in cases:
        base=ROOT/('evidence/enhanced-editor/dev83/native-reference' if '--repeat-track' in sys.argv else 'evidence/enhanced-editor/dev82/native-reference' if '--retrigger' in sys.argv else 'evidence/enhanced-editor/dev81/native-reference' if '--jump' in sys.argv else 'evidence/enhanced-editor/dev80/native-reference' if '--tempo' in sys.argv else 'evidence/enhanced-editor/dev79/native-reference' if '--flow' in sys.argv else 'evidence/enhanced-editor/dev78/native-reference' if '--tuning' in sys.argv else 'evidence/enhanced-editor/dev77/native-reference' if '--waveform' in sys.argv else 'evidence/enhanced-editor/dev76/native-reference' if '--tone-note' in sys.argv else 'evidence/enhanced-editor/dev75/native-reference' if '--tone' in sys.argv else 'evidence/enhanced-editor/dev74/native-reference' if '--modulation' in sys.argv else 'evidence/enhanced-editor/dev73/native-reference' if '--pitch' in sys.argv else 'evidence/enhanced-editor/dev72/native-reference' if '--silent' in sys.argv else 'evidence/enhanced-editor/dev71/native-reference' if '--cuts' in sys.argv else 'evidence/enhanced-editor/dev70/native-reference' if '--slides' in sys.argv else 'evidence/enhanced-editor/dev69/native-reference' if '--volume' in sys.argv else 'evidence/enhanced-editor/dev66/native')/('handoff_'+case)
        shutil.copyfile(str(base)+'.mod',run/(case+'.mod'));shutil.copyfile(str(base)+'0.log',run/(case+'.trace'))
        script += [name+' '+case+'.mod '+case+'.trace '+('1' if case in ['loop','return','ed','volume','clamp','up','down','fineup','finedown','cut0','cut3','silent','pitchup','pitchdown','finepup','finepdown','arp','vib','trem','vibvol','toneup','tonedown','tonevolup','tonevoldown','noteup','notedown','notevolup','notevoldown','vibramp','vibsquare','tremramp','tremsquare','tuneplus','tuneminus','glisson','glissoff','delay','loopflow','speed','tempo','jump','break','retrig2','retrig3','retrigzero','retriglater'] else '0')+' >'+case+'.log','Echo $RC >'+case+'.rc']
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
            assert (run/(case+'.log')).read_text().strip()=='HANDOFF renderer PASS'
            shutil.copyfile(run/(case+'.log'),out/(case+'.log'))
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        (out/'native-handoff.json').write_text(json.dumps({'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'binary':m['binaries'][name],'cases':cases,'stopped_audio':audio,'scope':'Reference-derived PCM, not live Paula capture or physical acceptance'},indent=2)+'\n');print('PASS: '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            if emu is None:
                matches=matching_socket()
                if len(matches)==1:emu=Emulator(matches[0])
            if emu is None:raise RuntimeError('Retain claim: no guarded socket')
            Emulator(emu.path).command('QUIT');process.wait(timeout=10)
if __name__=='__main__':main()
