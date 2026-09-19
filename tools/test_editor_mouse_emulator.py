#!/usr/bin/env python3
"""Use real Amiga input.device events against the Enhanced screen. Reserve first."""
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
    run=share/('mouse'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/mouse-evidence';out.mkdir(exist_ok=True)
    for n in ['PT24GEdit','PTEditorClick']:shutil.copyfile(ROOT/'build/dev'/n,run/n)
    shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',run/'input.mod')
    commands=[('edit-op',400,50),('mark',520,28),('copy',250,50),('unmark',250,87),('edit-back',400,87),('disk',400,70),('back',400,80),('play',250,10),('stop',400,10),('audition',250,87),('bottom-stop',510,502)]
    process=emu=None;start=time.monotonic()
    def log():return (run/'editor.log').read_text() if (run/'editor.log').exists() else ''
    def wait(condition,seconds=60):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            if condition():return
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            time.sleep(.1)
        raise RuntimeError('Mouse test deadline: '+str(run))
    try:
        lines=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,'Run >NIL: PT24GEdit input.mod >editor.log','Wait 5']
        for name,x,y in commands:lines += [f'PTEditorClick {x} {y} >{name}.log',f'Echo $RC >{name}.rc','Wait 3']
        lines+=['Echo done >clicks-done'];launch.write_text('\n'.join(lines)+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()),45);matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        wait(lambda:'COPIED 1 ROWS X 1 CHANNELS' in log());emu.command('SCREENSHOT',out/'00-mouse-block-controls.png')
        wait(lambda:'panel=2' in log());emu.command('SCREENSHOT',out/'01-mouse-disk-op.png')
        wait(lambda:'PLAYING SONG - PAULA CIA' in log());wait(lambda:'EDITOR REPLAY active=1' in log());emu.command('SCREENSHOT',out/'02-mouse-play.png')
        wait(lambda:'SAMPLE AUDITION - PAULA' in log());emu.command('SCREENSHOT',out/'03-mouse-audition.png')
        wait(lambda:(run/'clicks-done').exists());wait(lambda:log().count('status=STOPPED - AUDIO RELEASED')>=2)
        audio=emu.command('GET_AUDIO_STATE');assert all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
        results={}
        for name,_,_ in commands:
            results[name]={'rc':(run/(name+'.rc')).read_text().strip(),'log':(run/(name+'.log')).read_text()}
            assert results[name]['rc']=='0' and 'PROBE PASS' in results[name]['log'],results[name]
        emu.tap(0x45);wait(lambda:'EDITOR EXIT clean' in log());time.sleep(.5);emu.command('SCREENSHOT',out/'04-normal-exit.png')
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'binaries':{n:digest(run/n) for n in ['PT24GEdit','PTEditorClick']},'controls':results,'editor_log':log(),'stopped_audio':audio,'normal_exit':True,'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION']}}
        (out/'native-mouse.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS: input.device mouse events consumed by Edit Op, Mark, Copy, Unmark, Back, Disk Op, Play, Stop, Sample and bottom Stop; DMA off and normal exit')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Mouse test released:',run)
if __name__=='__main__':main()
