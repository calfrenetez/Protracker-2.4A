#!/usr/bin/env python3
"""Native file regressions; requires a separately coordinated shared030 window."""
import argparse, fcntl, hashlib, json, shutil, sys, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    group=parser.add_mutually_exclusive_group()
    group.add_argument('--studio-memory',choices=['mixer','sampler','song','editor','queued'],help='Run one production-allocator Studio fixture')
    group.add_argument('--input-memory',choices=['import','recent','exec-import','exec-recent'],help='Run one import or recent-file memory fixture')
    group.add_argument('--exec-memory',choices=['bounce','stems','failures','save'],help='Run one native Exec-backed memory fixture')
    group.add_argument('--stems-only',action='store_true',help='Run the allocated stem export test only')
    group.add_argument('--wav-only',action='store_true',help='Run the allocated WAV export test only')
    group.add_argument('--memory-failures-only',action='store_true',help='Run all WAV/stem allocation-failure checks only')
    group.add_argument('--bounce-only',action='store_true',help='Run the sample-bounce allocation regression only')
    group.add_argument('--allocated-only',action='store_true',help='Run the three allocated-path tests instead of the two legacy file tests')
    args=parser.parse_args()
    sys.path.insert(0,str(INFRA/'scripts'))
    from shared_guest import Guest
    out=ROOT/'build/dev'/('render-files-'+str(time.time_ns()));out.mkdir()
    result={'passed':False,'scope':'shared030 native render/stem file checks'}
    with (INFRA/'runtime/test.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        guest=Guest(INFRA,out);run=guest.share/out.name;run.mkdir()
        finished=False
        cases=[('render','PTRenderFileTest','RENDER FILE PASS:'),('stems','PTStemFileTest','STEMS files PASS:'),
               ('core-allocated','PTRenderAllocTest','RENDER PASS:'),
               ('render-allocated','PTRenderFileAllocTest','RENDER FILE PASS:'),
               ('stems-allocated','PTStemFileAllocTest','STEMS files PASS:'),
               ('bounce','PTBounceTest','BOUNCE PASS:'),
               ('memory-failures','PTRenderMemoryTest','RENDER MEMORY PASS:'),
               ('exec-bounce','PTExecBounceTest','BOUNCE PASS:'),
               ('exec-stems','PTExecStemsTest','STEMS files PASS:'),
               ('exec-failures','PTExecFailureTest','RENDER MEMORY PASS:'),
               ('exec-save','PTExecSaveTest','FILE SAVE MEMORY PASS:')]
        cases=[cases[7+['bounce','stems','failures','save'].index(args.exec_memory)]] if args.exec_memory else cases[4:5] if args.stems_only else cases[3:4] if args.wav_only else cases[6:7] if args.memory_failures_only else cases[5:6] if args.bounce_only else cases[2:5] if args.allocated_only else cases[:2]
        if args.input_memory:
            cases=[('import','PTFileLoadTest','FILE LOAD PASS')] if args.input_memory=='import' else [('recent','PTRecentMemoryTest','RECENT MEMORY PASS')]
            if args.input_memory=='exec-import':cases=[('import','PTExecImportTest','FILE LOAD PASS')]
            if args.input_memory=='exec-recent':cases=[('recent','PTExecRecentTest','RECENT MEMORY PASS')]
            result['scope']='shared030 native import/recent file checks'
        if args.studio_memory:
            cases=[{'mixer':('studio','PTExecStudioTest','STUDIO MIX PASS:'),
                    'sampler':('sampler-studio','PTExecSamplerStudioTest','SAMPLER STUDIO PASS:'),
                    'queued':('queued-song','PTExecQueuedSongTest','QUEUED SONG PASS:'),
                    'editor':('editor-studio','PTExecEditorStudioTest','EDITOR STUDIO PASS:'),
                    'song':('studio-song','PTExecStudioSongTest','SONG SESSION PASS:')}[args.studio_memory]]
            if args.studio_memory=='queued':cases.append(('studio-pump','PTExecStudioPumpTest','STUDIO PUMP PASS:'))
            result['scope']='shared030 Studio ownership core; no audio device transport'
        try:
            commands=['FailAt 21','Stack 65536']
            for name,binary,marker in cases:
                sub=run/name;sub.mkdir();shutil.copyfile(ROOT/'build/dev'/binary,sub/binary)
                result[binary+'_sha256']=hashlib.sha256((sub/binary).read_bytes()).hexdigest()
                commands+=['CD '+guest.device+run.name+'/'+name,binary+' '+guest.device+run.name+'/'+name+('/recent' if args.input_memory in ('recent','exec-recent') else '')+' >test.log','Echo $RC >test.rc']
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
                if args.studio_memory or args.exec_memory or args.input_memory in ('exec-import','exec-recent'):assert 'EXEC MEMORY PASS:' in log,log
            result['passed']=True
        finally:
            if finished:
                guest.launch.unlink();shutil.rmtree(run)
            result['run_files_cleaned']=finished
            (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(out)
if __name__=='__main__':main()
