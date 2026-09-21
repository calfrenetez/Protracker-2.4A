#!/usr/bin/env python3
"""Original pinned PT2.3F GUI load/edit/save test. Reserve private emulator first."""
import json,shutil,subprocess,time
from pathlib import Path
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket

def main():
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('classicconvert'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/classic-conversion-evidence'/run.name;out.mkdir(parents=True)
    baseline=json.loads((ROOT/'build/baseline/build.json').read_text())
    binary=ROOT/'build/baseline/PT2.3F';assert digest(binary)==baseline['binary_sha256']
    shutil.copyfile(binary,run/'PT2.3F');shutil.copyfile(ROOT/'build/baseline/PT.HELP',run/'PT.HELP')
    source=ROOT/'evidence/enhanced-editor/dev60/native/converted.mod';data=source.read_bytes();(run/'mod.converted').write_bytes(data)
    assert data[1080:1084]==b'M.K.' and data[1084]&15==1 and data[1085]==172
    assert int.from_bytes(data[42:44],'big')==8 and data[44]==1
    process=emu=None;start=time.monotonic();actions=[]
    def wait(check,seconds=45):
        until=min(start+300,time.monotonic()+seconds)
        while time.monotonic()<until:
            if check():return
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            time.sleep(.1)
        raise RuntimeError('Original tracker GUI deadline: '+str(run))
    def click(x,y):
        assert time.monotonic()-start<290;emu.click(x,y);actions.append(['click',x,y])
    def key(raw):
        assert time.monotonic()-start<290;emu.tap(raw);actions.append(['key',raw])
    def capture(name):
        path=out/name;emu.command('SCREENSHOT',path)
        wait(lambda:path.exists() and path.read_bytes().endswith(b'\0\0\0\0IEND\xaeB`\x82'))
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,'PT2.3F','Echo $RC >classic.rc'])+'\n')
        with (run/'emulator.log').open('wb') as log:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        # Original tracker detaches from its launcher. Its CLI return is not an
        # application-exit signal. Allow the private baseline to finish booting.
        wait(lambda:time.monotonic()-start>=35,45);capture('01-main.png');click(212,39)
        click(36,39)
        for _ in range(8):key(0x4f)
        for _ in range(63):key(0x46)
        keys=dict(zip('abcdefghijklmnopqrstuvwxyz',[0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31]));keys.update({str(i):i if i else 10 for i in range(10)})
        for ch in 'ptdev:'+run.name:
            if ch==':':
                emu.command('SEND_KEY',0x60,1)
                try:key(0x29)
                finally:emu.command('SEND_KEY',0x60,0)
            else:key(keys[ch])
        key(0x44);time.sleep(.5);capture('02-files.png')
        # This isolated directory has classic.rc, emulator.log, mod.converted,
        # PT.HELP and PT2.3F in the original alphabetical listing: module row3.
        click(128,61);time.sleep(.5);click(311,75);capture('03-loaded.png')
        key(0x40);key(0x32);key(0x40);capture('04-edited.png')
        click(212,39);click(273,5);capture('05-save-question.png');click(184,77)
        saved=run/'mod.PT BASELINE SMOKE';wait(lambda:saved.exists() and saved.stat().st_size==len(data))
        expected=bytearray(data);expected[1084]=(expected[1084]&240)|(381>>8);expected[1085]=381&255
        # Pinned PT2.3F.s LoadModule clears the first sample word at line17985.
        expected[2108:2110]=bytes(2)
        assert saved.read_bytes()==expected,'Unexpected original-tracker save difference'
        assert (run/'mod.converted').read_bytes()==data
        capture('06-saved.png');shutil.copyfile(source,out/'input.mod');shutil.copyfile(saved,out/'saved.mod')
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),'original_tracker_sha256':digest(binary),'source_commit':baseline['source_commit'],'input_sha256':digest(source),'saved_sha256':digest(saved),'note_period_before':428,'note_period_after':381,'upstream_sample_first_word_zeroed':True,'all_other_bytes_preserved':True,'input_preserved':True,'saved_mod_verified':True,'physical_tested':False,'actions':actions}
        (out/'native-classic-conversion.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS: '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            if emu is None:
                matches=matching_socket()
                if len(matches)==1:emu=Emulator(matches[0])
            if emu is None:raise RuntimeError('Retain claim: no guarded emulator socket')
            Emulator(emu.path).command('QUIT');process.wait(timeout=10)
if __name__=='__main__':main()
