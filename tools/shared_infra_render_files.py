#!/usr/bin/env python3
"""Native file regressions; requires a separately coordinated shared030 window."""
import fcntl, hashlib, json, shutil, sys, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
def main():
    sys.path.insert(0,str(INFRA/'scripts'))
    from shared_guest import Guest
    out=ROOT/'build/dev'/('render-files-'+str(time.time_ns()));out.mkdir()
    result={'passed':False,'scope':'shared030 native render/stem file checks'}
    with (INFRA/'runtime/test.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        guest=Guest(INFRA,out);run=guest.share/out.name;run.mkdir()
        finished=False
        cases=[('render','PTRenderFileTest','RENDER FILE PASS:'),('stems','PTStemFileTest','STEMS files PASS:')]
        try:
            commands=['FailAt 21','Stack 65536']
            for name,binary,marker in cases:
                sub=run/name;sub.mkdir();shutil.copyfile(ROOT/'build/dev'/binary,sub/binary)
                result[binary+'_sha256']=hashlib.sha256((sub/binary).read_bytes()).hexdigest()
                commands+=['CD '+guest.device+run.name+'/'+name,binary+' '+guest.device+run.name+'/'+name+' >test.log','Echo $RC >test.rc']
            commands+=['Echo done >'+guest.device+run.name+'/done']
            guest.launch.write_text('\n'.join(commands)+'\n');guest.start()
            deadline=time.monotonic()+90
            while not (run/'done').exists():
                if time.monotonic()>deadline:raise RuntimeError('Native file checks timed out; preserve owned run for recovery')
                time.sleep(.2)
            finished=True
            for name,binary,marker in cases:
                log=(run/name/'test.log').read_text();(out/(name+'.log')).write_text(log)
                result[name+'_returncode']=(run/name/'test.rc').read_text().strip()
                assert result[name+'_returncode']=='0' and marker in log,log
            result['passed']=True
        finally:
            if finished:
                guest.launch.unlink();shutil.rmtree(run)
            result['run_files_cleaned']=finished
            (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(out)
if __name__=='__main__':main()
