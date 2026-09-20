#!/usr/bin/env python3
"""Compare native reference PCM against retained sample-range evidence. Reserve Amiberry first."""
import json,shutil,subprocess,tempfile,time
from pathlib import Path
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket
from make_offset_fixtures import fixtures

def main():
    assert not matching_socket() and not Path('/tmp/amiberry.sock').exists()
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('offsetpcm'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/offset-render-evidence'/run.name;out.mkdir(parents=True)
    shutil.copyfile(ROOT/'build/dev/PTOffsetRenderTest',run/'PTOffsetRenderTest')
    evidence=ROOT/'evidence/enhanced-editor/dev39/native';commands=[('safety',[])];inputs={}
    for name,data,meta in fixtures():
        for suffix in ['.mod','0.log']:
            source=evidence/(name+suffix);shutil.copyfile(source,run/source.name);inputs[source.name]=digest(source)
        for ch in range(4 if name=='offset_four' else 1):commands.append((name+'-ch'+str(ch),[name+'.mod',name+'0.log',str(ch)]))
    assert {name+'.log' for name,args in commands}.isdisjoint(inputs)
    with tempfile.TemporaryDirectory() as tmp:
        host=Path(tmp)/'offset'
        sources=['tests/render_offset_test.c',*['src/core/'+n+'.c' for n in ['render','pitch','timeline','frame_clock','flow','voice','document','pp20','mod_project','mod_inspect','project','channels','pcm']]]
        subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*sources,'-o',str(host)],cwd=ROOT,check=True)
        expected={name:subprocess.check_output([str(host),*args],cwd=run,text=True) for name,args in commands}
    script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name]
    for name,args in commands:script+=['PTOffsetRenderTest '+ ' '.join(args)+' >'+name+'.log','Echo $RC >'+name+'.rc']
    script+=['Echo '+run.name+' >done'];process=emu=None;start=time.monotonic()
    try:
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        last=-1
        while time.monotonic()-start<480:
            assert process.poll() is None,'Emulator exited'
            if not emu:
                matches=matching_socket();assert len(matches)<=1
                if matches:emu=Emulator(matches[0])
            for rc in run.glob('*.rc'):
                value=rc.read_text().strip()
                if value and value!='0':raise RuntimeError('Native check failed: '+rc.name)
            complete=len(list(run.glob('*.rc')))
            if complete!=last:print(f'Native checks: {complete}/{len(commands)}',flush=True);last=complete
            if (run/'done').exists() and (run/'done').read_text().strip()==run.name:break
            time.sleep(.3)
        else:raise RuntimeError('Native PCM deadline')
        logs={}
        for name,args in commands:
            logs[name]=(run/(name+'.log')).read_text();assert (run/(name+'.rc')).read_text().strip()=='0',(name,logs[name]);assert logs[name]==expected[name]
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'binary_sha256':digest(run/'PTOffsetRenderTest'),'input_sha256':inputs,'logs':logs,'stopped_audio':audio,'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']},'scope':'Reference immutable PCM at 48k/rate428 against dev39 native ranges; nonloops stop after initial segment. No native sample-word mutation, analogue audio or physical acceptance.'}
        (out/'native-offset-render.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS: '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Offset renderer released:',run,flush=True)
if __name__=='__main__':main()
