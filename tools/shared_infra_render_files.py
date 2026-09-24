#!/usr/bin/env python3
"""Native file regressions; requires a separately coordinated shared030 window."""
import argparse, fcntl, hashlib, importlib.util, json, shutil, sys, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    group=parser.add_mutually_exclusive_group()
    group.add_argument('--render-cli',metavar='REFERENCE_WAV',help='Run one-row native renderer against an exact host reference')
    group.add_argument('--pp20-import',action='store_true',help='Run native converter bounded packed MOD import')
    group.add_argument('--project-import',action='store_true',help='Run native converter enhanced-project load/save roundtrip')
    group.add_argument('--mod-import',action='store_true',help='Run bounded MOD document load with native Fast allocator')
    group.add_argument('--sample-import',choices=['raw','wav','svx'],help='Run streamed sample import with native Fast allocator')
    group.add_argument('--mod-stream',action='store_true',help='Run bounded classic MOD export with native Fast allocator')
    group.add_argument('--project-stream',action='store_true',help='Run streamed master project save with native Fast allocator')
    group.add_argument('--sample-svx',action='store_true',help='Run streamed master IFF export with native Fast allocator')
    group.add_argument('--sample-raw',action='store_true',help='Run streamed master RAW export with native Fast allocator')
    group.add_argument('--sample-wav',action='store_true',help='Run streamed master WAV export with native Fast allocator')
    group.add_argument('--amigus-discovery',action='store_true',help='Discovery-only native library probe; no reservation or MMIO')
    group.add_argument('--studio-memory',choices=['mixer','sampler','song','editor','queued','consumer','fifo','session','register-session','reserved-session'],help='Run one production-allocator Studio fixture')
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
                    'reserved-session':('reserved-session','PTExecAmiGusReservedSessionTest','AMIGUS RESERVED SESSION PASS:'),
                    'register-session':('register-session','PTExecAmiGusRegisterSessionTest','AMIGUS REGISTER SESSION PASS:'),
                    'session':('amigus-session','PTExecAmiGusSessionTest','AMIGUS SESSION PASS:'),
                    'fifo':('amigus-chain','PTExecAmiGusChainTest','AMIGUS CHAIN PASS:'),
                    'consumer':('studio-consumer','PTExecStudioConsumerTest','STUDIO CONSUMER PASS:'),
                    'queued':('queued-song','PTExecQueuedSongTest','QUEUED SONG PASS:'),
                    'editor':('editor-studio','PTExecEditorStudioTest','EDITOR STUDIO PASS:'),
                    'song':('studio-song','PTExecStudioSongTest','SONG SESSION PASS:')}[args.studio_memory]]
            if args.studio_memory=='queued':cases.append(('studio-pump','PTExecStudioPumpTest','STUDIO PUMP PASS:'))
            if args.studio_memory=='consumer':cases.append(('queued-song','PTExecQueuedSongTest','QUEUED SONG PASS:'))
            result['scope']='shared030 Studio ownership core; no audio device transport'
        if args.sample_wav:
            cases=[('sample-wav','PTExecSampleFileTest','SAMPLE WAV STREAM PASS:')]
            result['scope']='shared030 streamed master WAV export and Exec Fast allocator'
        if args.sample_raw:
            cases=[('sample-raw','PTExecSampleRawFileTest','SAMPLE RAW STREAM PASS:')]
            result['scope']='shared030 streamed master RAW export and Exec Fast allocator'
        if args.sample_svx:
            cases=[('sample-svx','PTExecSampleSvxFileTest','SAMPLE SVX STREAM PASS:')]
            result['scope']='shared030 streamed master IFF export and Exec Fast allocator'
        if args.render_cli:
            cases=[('render-cli','PT24GRender','WAV frames=')]
            result['scope']='shared030 Fast-allocator renderer, one-row24-bit WAV host parity'
        if args.pp20_import:
            cases=[('pp20-import','PT24GConvert','SAVED format=MOD')]
            result['scope']='shared030 bounded PP20 converter load and exact MOD export'
        if args.project_import:
            cases=[('project-import','PT24GConvert','SAVED format=PT24G-v1')]
            result['scope']='shared030 bounded enhanced-project converter load/save; exact mixed master roundtrip'
        if args.mod_import:
            cases=[('mod-import','PTExecModImportTest','MOD IMPORT STREAM PASS:')]
            result['scope']='shared030 streamed MOD document import and Exec Fast allocator'
        if args.sample_import:
            cases=[('raw-import','PTExecRawImportTest','RAW IMPORT STREAM PASS:')] if args.sample_import=='raw' else [('svx-import','PTExecSvxImportTest','SVX IMPORT STREAM PASS:')] if args.sample_import=='svx' else [('wav-import','PTExecWavImportTest','WAV IMPORT STREAM PASS:')]
            result['scope']='shared030 streamed sample import and Exec Fast allocator'
        if args.mod_stream:
            cases=[('mod-stream','PTExecModStreamTest','MOD STREAM PASS:')]
            result['scope']='shared030 streamed master MOD export and Exec Fast allocator'
        if args.project_stream:
            cases=[('project-stream','PTExecProjectStreamTest','PROJECT STREAM PASS:')]
            result['scope']='shared030 streamed master project save and Exec Fast allocator'
        if args.amigus_discovery:
            cases=[('amigus-discovery','PTAmiGusDiscovery','AMIGUS DISCOVERY PASS:')]
            result['scope']='shared030 discovery-only native amigus.library probe; no reservation or MMIO'
        try:
            commands=['FailAt 21','Stack 65536']
            for name,binary,marker in cases:
                sub=run/name;sub.mkdir();shutil.copyfile(ROOT/'build/dev'/binary,sub/binary)
                result[binary+'_sha256']=hashlib.sha256((sub/binary).read_bytes()).hexdigest()
                commands+=['CD '+guest.device+run.name+'/'+name,binary+' '+guest.device+run.name+'/'+name+('/module.mod' if args.mod_import else '/sample.input' if args.sample_import else '/master.mod' if args.mod_stream else '/master.ptg' if args.project_stream else '/sample.iff' if args.sample_svx else '/sample.raw' if args.sample_raw else '/sample.wav' if args.sample_wav else '/recent' if args.input_memory in ('recent','exec-recent') else '')+' >test.log','Echo $RC >test.rc']
            if args.project_import:
                sub=run/'project-import';shutil.copyfile(ROOT/'tests/fixtures/project-v1/mixed.ptg',sub/'source.ptg')
                commands=['FailAt 21','Stack 65536','CD '+guest.device+run.name+'/project-import',
                          'PT24GConvert project source.ptg copy.ptg >test.log','Echo $RC >test.rc']
            if args.pp20_import:
                spec=importlib.util.spec_from_file_location('packer',ROOT/'tools/make_pp20_fixture.py')
                packer=importlib.util.module_from_spec(spec);spec.loader.exec_module(packer)
                sub=run/'pp20-import';(sub/'source.pp').write_bytes(packer.literal((ROOT/'evidence/baseline/mod.baseline').read_bytes()))
                commands=['FailAt 21','Stack 65536','CD '+guest.device+run.name+'/pp20-import',
                          'PT24GConvert mod source.pp copy.mod >test.log','Echo $RC >test.rc']
            if args.render_cli:
                sub=run/'render-cli';shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',sub/'source.mod')
                commands=['FailAt 21','Stack 65536','CD '+guest.device+run.name+'/render-cli',
                          'PT24GRender source.mod output.wav --pattern 0 --from-row 0 --to-row 1 --tracks 1 >test.log','Echo $RC >test.rc']
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
                if args.mod_import or args.sample_import or args.mod_stream or args.project_stream or args.sample_svx or args.sample_raw or args.sample_wav or args.studio_memory or args.exec_memory or args.input_memory in ('exec-import','exec-recent'):assert 'EXEC MEMORY PASS:' in log,log
            if args.mod_import or args.sample_import or args.mod_stream or args.project_stream or args.sample_svx or args.sample_wav or args.sample_raw:
                directory,binary=('mod-import','PTExecModImportTest') if args.mod_import else ('svx-import','PTExecSvxImportTest') if args.sample_import=='svx' else ('raw-import','PTExecRawImportTest') if args.sample_import=='raw' else ('wav-import','PTExecWavImportTest') if args.sample_import=='wav' else ('mod-stream','PTExecModStreamTest') if args.mod_stream else ('project-stream','PTExecProjectStreamTest') if args.project_stream else ('sample-svx','PTExecSampleSvxFileTest') if args.sample_svx else ('sample-raw','PTExecSampleRawFileTest') if args.sample_raw else ('sample-wav','PTExecSampleFileTest')
                remaining=sorted(p.name for p in (run/directory).iterdir())
                assert remaining==[binary,'test.log','test.rc'],remaining
                result['sample_staging_clean']=True
            if args.project_import:
                sub=run/'project-import';expected=(ROOT/'tests/fixtures/project-v1/mixed.ptg').read_bytes()
                assert (sub/'copy.ptg').read_bytes()==expected and (sub/'source.ptg').read_bytes()==expected
                assert sorted(p.name for p in sub.iterdir())==['PT24GConvert','copy.ptg','source.ptg','test.log','test.rc']
                result['exact_master_roundtrip']=True
                result['fixture_sha256']=hashlib.sha256(expected).hexdigest()
            if args.pp20_import:
                sub=run/'pp20-import';expected=(ROOT/'evidence/baseline/mod.baseline').read_bytes()
                assert (sub/'copy.mod').read_bytes()==expected
                assert (sub/'source.pp').read_bytes()==packer.literal(expected)
                assert sorted(p.name for p in sub.iterdir())==['PT24GConvert','copy.mod','source.pp','test.log','test.rc']
                result['exact_master_roundtrip']=True
                result['fixture_sha256']=hashlib.sha256(expected).hexdigest()
            if args.render_cli:
                sub=run/'render-cli';expected=Path(args.render_cli).read_bytes()
                assert (sub/'output.wav').read_bytes()==expected
                assert (sub/'source.mod').read_bytes()==(ROOT/'evidence/baseline/mod.baseline').read_bytes()
                assert sorted(p.name for p in sub.iterdir())==['PT24GRender','output.wav','source.mod','test.log','test.rc']
                result['exact_host_wav']=True
                result['reference_sha256']=hashlib.sha256(expected).hexdigest()
            result['passed']=True
        finally:
            if finished:
                guest.launch.unlink();shutil.rmtree(run)
            result['run_files_cleaned']=finished
            (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(out)
if __name__=='__main__':main()
