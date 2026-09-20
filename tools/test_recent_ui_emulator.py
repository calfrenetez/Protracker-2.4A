#!/usr/bin/env python3
"""Reserved private Amiberry window only: Recent Projects UI and persistence."""
import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess
import time
import zlib
from build_diagnostic import ROOT,digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket


def record(paths,generation=1):
    payload=b''.join(struct.pack('>H',len(p.encode()))+p.encode() for p in paths)
    data=bytearray(struct.pack('>4sHHII',b'PTRC',1,len(paths),16+len(payload),0)+payload)
    data[12:16]=struct.pack('>I',zlib.crc32(data))
    return struct.pack('>II',generation,generation^0xffffffff)+data


def decode(path):
    data=bytearray(path.read_bytes());generation,complement=struct.unpack('>II',data[:8]);assert generation^complement==0xffffffff
    data=data[8:];magic,version,count,size,checksum=struct.unpack('>4sHHII',data[:16]);assert magic==b'PTRC' and version==1 and size==len(data)
    data[12:16]=bytes(4);assert zlib.crc32(data)==checksum
    pos=16;paths=[]
    for _ in range(count):
        n=int.from_bytes(data[pos:pos+2],'big');pos+=2;paths.append(data[pos:pos+n].decode());pos+=n
    assert pos==len(data)
    return generation,paths


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--ui-only',action='store_true');args=parser.parse_args()
    processes=subprocess.check_output(['ps','ax','-o','pid=,comm='],text=True)
    assert not any(line.lower().endswith('/amiberry') for line in processes.splitlines())
    holders=subprocess.run(['lsof',str(ROOT/'local/baseline.hdf')],capture_output=True,text=True)
    assert holders.returncode==1 and not holders.stdout
    if matching_socket() or Path('/tmp/amiberry.sock').exists():raise SystemExit('Emulator already owned')
    manifest=json.loads((ROOT/'build/dev/core-build.json').read_text())
    assert all(digest(ROOT/p)==h for p,h in manifest['sources'].items())
    env=json.loads((ROOT/'local/environment.json').read_text());share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    assert digest(launch)=='9c801b94c85b06cd124fe2d611f185bfe135c8c7442f8263a761c79102f8ddf3'
    run=share/('recentui'+str(time.time_ns()));run.mkdir();out=ROOT/'build/dev/recent-ui-evidence'/run.name;out.mkdir(parents=True)
    names=['PT24GEdit','PT24GConvert','PTEditorClick','PTRecentTest','PTRecentFileTest']
    for name in names:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    fixture=ROOT/'evidence/enhanced-editor/dev48/native/fine_override.mod';shutil.copyfile(fixture,run/'input.mod')
    (run/'bad.ptg').write_bytes(b'not a project')
    base='RAM:'+run.name+'/';missing=base+'missing.mod';bad=base+'bad.ptg';input_path=base+'input.mod';saved=base+'saved.ptg'
    (run/'seed.0').write_bytes(record([missing,bad]))
    current='first.log';process=emu=None;start=time.monotonic()
    def log():return (run/current).read_text() if (run/current).exists() else ''
    def wait(check,seconds=180):
        end=min(time.monotonic()+seconds,start+600)
        while time.monotonic()<end:
            if check():return
            if process.poll() is not None:raise RuntimeError('Emulator exited: '+str(run))
            time.sleep(.1)
        raise RuntimeError('Recent UI deadline '+str(run)+' '+log()[-1200:])
    def frame(text,after=0):wait(lambda:any('EDITOR FRAME' in line and text in line for line in log()[after:].splitlines()))
    def key(raw,control=False,shift=False):
        offset=len(log())
        if control:emu.command('SEND_KEY',0x63,1)
        if shift:emu.command('SEND_KEY',0x60,1)
        try:emu.tap(raw)
        finally:
            if shift:emu.command('SEND_KEY',0x60,0)
            if control:emu.command('SEND_KEY',0x63,0)
        wait(lambda:'EDITOR FRAME' in log()[offset:] or 'EDITOR EXIT clean' in log()[offset:]);return offset
    click_index=0
    click_targets=[(500,40),(300,75),(300,85),(300,85)]
    def click(x,y):
        nonlocal click_index
        assert (x,y)==click_targets[click_index]
        click_index+=1;offset=len(log());(run/('click%d.go'%click_index)).write_text('go\n')
        result=run/('click%d.rc'%click_index)
        wait(lambda:result.exists() and result.read_text().strip()!='')
        assert result.read_text().strip()=='0'
        wait(lambda:'EDITOR FRAME' in log()[offset:]);return offset
    def recent():frame('RECENT: SELECT THEN OPEN',key(0x13,True,True))
    clicks=['FailAt 21','Stack 65536']
    for i,(x,y) in enumerate(click_targets,1):
        prefix='PTDEV:'+run.name+'/click'+str(i)
        clicks += ['Lab wait'+str(i),'If NOT EXISTS '+prefix+'.go','Wait 1','Skip wait'+str(i)+' BACK','EndIf',
            'PTDEV:'+run.name+'/PTEditorClick '+str(x)+' '+str(y)+' >'+prefix+'.log','Echo $RC >'+prefix+'.rc']
    clicks += ['Echo complete >PTDEV:'+run.name+'/clicks.done']
    (run/'clicks').write_text('\n'.join(clicks)+'\n')
    try:
        script=['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,'MakeDir RAM:'+run.name,
            'Copy input.mod '+input_path,'Copy bad.ptg '+bad,'Copy seed.0 '+base+'prefs.0',
            *([] if args.ui_only else ['PTRecentTest >core.log','Echo $RC >core.rc','PTRecentFileTest '+base+'testprefs >file.log','Echo $RC >file.rc']),
            'PT24GConvert project input.mod baseline.ptg >convert.log','Echo $RC >convert.rc',
            'SetEnv PT24G_RECENT_PREFIX '+base+'prefs',
            'Run >NIL: Execute PTDEV:'+run.name+'/clicks >PTDEV:'+run.name+'/clicks.log',
            'PT24GEdit '+input_path+' '+saved+' >first.log','Echo $RC >first.rc',
            'Copy '+base+'prefs.0 first.0','Copy '+base+'prefs.1 first.1','Copy '+saved+' saved.ptg',
            'PT24GEdit >second.log','Echo $RC >second.rc',
            'Copy '+base+'prefs.0 second.0','Copy '+base+'prefs.1 second.1',
            'PT24GEdit >third.log','Echo $RC >third.rc',
            'List '+base+' ALL >ram-list.log','Echo '+run.name+' >done']
        launch.write_text('\n'.join(script)+'\n')
        with (run/'emulator.log').open('wb') as f:process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
        wait(lambda:bool(matching_socket()));matches=matching_socket();assert len(matches)==1;emu=Emulator(matches[0])
        frame('status=READY -');assert 'EDITOR RECENT saved=1 count=3' in log()
        assert all((run/(name+'.rc')).read_text().strip()=='0' for name in (['convert'] if args.ui_only else ['core','file','convert']))
        key(0x40);key(0x32);frame('dirty=1')
        recent();key(0x4d);frame('UNSAVED EDITS',key(0x44));frame('dirty=1 status=LOAD FAILED',key(0x44))
        key(0x4d);frame('UNSAVED EDITS',key(0x44));frame('dirty=1 status=LOAD FAILED',key(0x44))
        assert log().count('EDITOR RECENT saved=')==1
        key(0x4c);key(0x4c);frame('UNSAVED EDITS',key(0x44));frame('dirty=0 status=PROJECT LOADED',key(0x44))
        frame('dirty=0 status=PROJECT SAVED',key(0x21,True));assert 'EDITOR RECENT saved=1 count=4' in log()
        count=log().count('EDITOR RECENT saved=');frame('SAVE REFUSED',key(0x21,True));assert log().count('EDITOR RECENT saved=')==count
        key(0x45);current='second.log';frame('status=READY -')
        assert (run/'saved.ptg').read_bytes()==(run/'baseline.ptg').read_bytes()
        first=max(decode(run/'first.0'),decode(run/'first.1'));assert len(first[1])==4 and first[1][2:]==[missing,bad],first
        assert [p.split(':',1)[1] for p in first[1][:2]]==[run.name+'/saved.ptg',run.name+'/input.mod'],first
        recent();shot=out/(run.name+'.png');emu.command('SCREENSHOT',shot)
        wait(lambda:shot.exists() and shot.read_bytes().endswith(b'\0\0\0\0IEND\xaeB`\x82'));shot.rename(out/'recent-projects.png')
        frame('PROJECT LOADED',key(0x44));recent()
        frame('RECENT LIST UPDATED',click(500,40));assert 'EDITOR RECENT saved=1 count=3' in log()
        frame('RECENT LIST UPDATED',click(300,75));assert 'EDITOR RECENT saved=1 count=0' in log()
        key(0x45);click(300,85);key(0x45);current='third.log';frame('status=READY -')
        second=max(decode(run/'second.0'),decode(run/'second.1'));assert second[1]==[]
        recent();frame('RECENT LIST IS EMPTY',key(0x44));key(0x45);click(300,85);key(0x45)
        wait(lambda:(run/'done').exists() and (run/'clicks.done').exists());assert all((run/(n+'.rc')).read_text().strip()=='0' for n in ['first','second','third'])
        report={'run_id':run.name,'core_tests_run':not args.ui_only,'native_mouse_delivery_verified':True,'elapsed_seconds':round(time.monotonic()-start,3),'first_preferences':first,
            'cleared_preferences':second,'missing_corrupt_open_preserves_dirty_project':True,
            'saved_project_exact_baseline':True,'failed_save_excluded':True,'relaunch_and_recent_open':True,
            'binaries':{n:digest(run/n) for n in names},'environment':{c:emu.command(c) for c in ['GET_STATUS','GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-recent-ui.json').write_text(json.dumps(report,indent=2)+'\n')
        print('PASS: Recent Projects persistence, failed-open/save exclusion, dirty confirmation, reopen/remove/clear; '+str(out),flush=True)
    finally:
        for pattern in ['*.log','*.rc','*.0','*.1','*.ptg']:
            for source in run.glob(pattern):shutil.copyfile(source,out/source.name)
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu:Emulator(emu.path).command('QUIT')
                else:process.terminate()
            finally:process.wait(timeout=10)
        print('Recent test released:',run,flush=True)
if __name__=='__main__':main()
