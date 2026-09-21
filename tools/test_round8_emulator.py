#!/usr/bin/env python3
"""Native precision-conversion tests. Reserve the private emulator first."""
import json,shutil,subprocess,time
from pathlib import Path
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket

def main():
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    manifest=json.loads((ROOT/'build/dev/core-build.json').read_text())
    assert all(digest(ROOT/p)==h for p,h in manifest['sources'].items())
    programs=['PTModRound8Test','PT24GConvert','PTSampleTraceTest']
    for n in programs:assert digest(ROOT/'build/dev'/n)==manifest['binaries'][n]['sha256']
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('round8'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/round8-evidence'/run.name;out.mkdir(parents=True)
    for n in programs:shutil.copyfile(ROOT/'build/dev'/n,run/n)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'baseline.mod')
    sources=['src/core/document.c','src/core/pp20.c','src/core/safe_save.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
    subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-Isrc/core','tests/mod_round8_test.c',*sources,'-o',str(out/'host-test')],cwd=ROOT,check=True)
    subprocess.run([str(out/'host-test'),str(run/'baseline.mod'),str(out)],check=True)
    process=emu=None;start=time.monotonic()
    commands=['PTModRound8Test baseline.mod PTDEV:'+run.name+' >core.log','Echo $RC >core.rc',
        'PT24GConvert mod high.ptg refused.mod >strict.log','Echo $RC >strict.rc',
        'PT24GConvert mod8 high.ptg output.mod >converted.log','Echo $RC >converted.rc',
        'PT24GConvert mod8 high.ptg output.mod >existing.log','Echo $RC >existing.rc',
        'PT24GConvert project output.mod reopened.ptg >reopen.log','Echo $RC >reopen.rc',
        'PTSampleTraceTest output.mod 64 >trace.log','Echo $RC >trace.rc',
        'PT24GConvert mod8tpdf high.ptg tpdf.mod >tpdf.log','Echo $RC >tpdf.rc',
        'PT24GConvert mod8tpdf high.ptg tpdf.mod >tpdfexisting.log','Echo $RC >tpdfexisting.rc',
        'PTSampleTraceTest tpdf.mod 64 >tpdftrace.log','Echo $RC >tpdftrace.rc']
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,*commands,'Echo '+run.name+' >done'])+'\n')
        with (run/'emulator.log').open('wb') as log:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        while time.monotonic()-start<180:
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            if emu is None:
                matches=matching_socket()
                if len(matches)>1:raise RuntimeError('Ambiguous emulator')
                if matches:emu=Emulator(matches[0])
            if (run/'done').exists():break
            time.sleep(.2)
        else:raise RuntimeError('Native conversion deadline: '+str(run))
        assert (run/'done').read_text().strip()==run.name
        cases={}
        for n,expected in [('core',0),('strict',20),('converted',0),('existing',20),('reopen',0),('trace',0),('tpdf',0),('tpdfexisting',20),('tpdftrace',0)]:
            rc=int((run/(n+'.rc')).read_text().strip());log=(run/(n+'.log')).read_text();assert rc==expected,(n,rc,log)
            cases[n]={'rc':rc,'log':log};shutil.copyfile(run/(n+'.log'),out/(n+'.log'))
        assert 'result=CONVERTED remaining_issues=0x0' in cases['converted']['log']
        assert (run/'output.mod').read_bytes()==(run/'expected.mod').read_bytes()==(out/'expected.mod').read_bytes()
        assert 'dither=tpdf-fixed result=CONVERTED' in cases['tpdf']['log']
        assert (run/'tpdf.mod').read_bytes()==(run/'dithered.mod').read_bytes()==(out/'dithered.mod').read_bytes()
        assert (run/'high.ptg').read_bytes()==(out/'high.ptg').read_bytes()
        assert not (run/'refused.mod').exists() and not list(run.glob('*.pttmp-*'))
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'binaries':{n:manifest['binaries'][n] for n in programs},'cases':cases,'source_preserved':True,'host_native_exact':True,'stopped_audio':audio,'original_tracker_gui_tested':False,'physical_tested':False}
        (out/'native-round8.json').write_text(json.dumps(report,indent=2)+'\n')
        for n in ['output.mod','tpdf.mod','reopened.ptg']:shutil.copyfile(run/n,out/n)
        print('PASS: '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            if emu is None:
                matches=matching_socket()
                if len(matches)==1:emu=Emulator(matches[0])
            if emu is None:raise RuntimeError('Retain claim: guarded emulator socket unavailable')
            Emulator(emu.path).command('QUIT');process.wait(timeout=10)
if __name__=='__main__':main()
