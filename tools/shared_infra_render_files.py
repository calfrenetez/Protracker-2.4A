#!/usr/bin/env python3
"""Native file regressions; requires a separately coordinated shared030 window."""
import argparse, fcntl, hashlib, importlib.util, json, os, shutil, sys, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
def acquire_shared_lock(lock,out,result):
    """Refuse before any Guest creation, recording a busy window accurately."""
    try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    except BlockingIOError:
        result.update(prelaunch_refused=True,run_files_staged=False,
                      refusal_reason='Shared test lock busy; no guest command or retry')
        (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
        print(out)
        raise
def require_running_guest(guest,out,phase):
    """Read only: a connected bridge/IPC can belong to a paused emulator."""
    status=guest.command('GET_STATUS')
    (out/('guest-status-'+phase+'.json')).write_text(json.dumps({'status':status},indent=2)+'\n')
    paused=[field for field in status.split('\t') if field.startswith('Paused=')]
    if paused!=['Paused=false']:
        raise RuntimeError('Shared guest is paused or its running state is unknown; no automatic resume/retry')
def prepare_run(guest,out):
    require_running_guest(guest,out,'before-staging')
    run=guest.share/out.name;run.mkdir();return run
def finish_run(guest,run,out,result,finished,guard_audio):
    """Report cleanup only after absence; retain evidence and raise on uncertainty.

    Called with the shared lock held. Never retry removal or clean incomplete runs.
    """
    result['run_files_cleaned']=False
    try:
        if not finished:return
        if guard_audio:
            state=guest.command('GET_AUDIO_STATE')
            result['cleanup_audio']=state
            if not all('ch%d_dma=0'%i in state.split('\t') for i in range(4)):
                raise RuntimeError('Completed guest still has active or unknown audio DMA; cleanup refused')
        guest.launch.unlink(missing_ok=True)
        shutil.rmtree(run)
        # An emulator/shared filesystem can recreate an empty directory after
        # deletion. lexists also refuses dangling links; never delete a second time.
        if os.path.lexists(run) or os.path.lexists(guest.launch):
            raise RuntimeError('Completed run or launcher remains after cleanup; inspect before release')
        result['run_files_cleaned']=True
    except Exception as error:
        result['passed']=False
        result['cleanup_error']=str(error)
        raise
    finally:
        (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
        print(out)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    group=parser.add_mutually_exclusive_group()
    group.add_argument('--cia-timing',action='store_true',help='Finite owned CIA timer IRQ timestamp diagnostic; no Paula writes')
    group.add_argument('--invert-stem-cli',metavar='REFERENCE_DIRECTORY',help='Verify bounded shared-sample EFx native stems against host bytes')
    group.add_argument('--invert-cli',metavar='REFERENCE_WAV',help='Verify bounded EFx CLI WAV against host bytes')
    group.add_argument('--invert-bounce',action='store_true',help='Run EFx sample-bounce transaction using native Fast allocator')
    group.add_argument('--invert-editor',action='store_true',help='Run editor EFx stop/lease guards using native Fast allocator')
    group.add_argument('--invert-sampler',action='store_true',help='Run sampler EFx version guards using native Fast allocator')
    group.add_argument('--invert-session',action='store_true',help='Run queued private EFx ownership using native Fast allocator')
    group.add_argument('--invert-render',action='store_true',help='Run bounded offline EFx PCM and failure checks')
    group.add_argument('--stem-cli',metavar='REFERENCE_DIRECTORY',help='Run two native stems and existing-directory refusal')
    group.add_argument('--render-cli',metavar='REFERENCE_WAV',help='Run one-row native renderer against an exact host reference')
    group.add_argument('--pp20-import',action='store_true',help='Run native converter bounded packed MOD import')
    group.add_argument('--project-import',action='store_true',help='Run native converter enhanced-project load/save roundtrip')
    group.add_argument('--recovery-file',action='store_true',help='Run explicit full-precision recovery snapshot/restore transactions')
    group.add_argument('--capture-memory',action='store_true',help='Run synthetic recording staging/publication with native Fast allocator; no device capture')
    group.add_argument('--capture-session-memory',action='store_true',help='Run injected recording ownership and stop/quiescence with native Fast allocator; no device input')
    group.add_argument('--amigus-capture-memory',action='store_true',help='Run injected recording PCM/interrupt ownership with native Fast allocator; no card input')
    group.add_argument('--editor-capture-memory',action='store_true',help='Run editor recording barriers and publication with native Fast allocator; no device input')
    group.add_argument('--paula-memory',choices=['cache','voices','preflight','mixed','mixed-owner','dispatch','song','editor','editor-mixed','reservation','output','output-diagnostic','engine','prepared-output','transport','wait','wait-latency','wait-priority','paula-wait-priority','paula-boundary'],help='Run routed Paula ownership/capability with native allocators and injected readers; no DMA')
    group.add_argument('--sample-dispatch',action='store_true',help='Check bounded sample dispatch and24-bit precision with Exec allocator')
    group.add_argument('--source-memory',action='store_true',help='Run donor ownership/failure/undo checks with native Fast allocator')
    group.add_argument('--mod-import',action='store_true',help='Run bounded MOD document load with native Fast allocator')
    group.add_argument('--sample-import',choices=['raw','wav','svx'],help='Run streamed sample import with native Fast allocator')
    group.add_argument('--mod-stream',action='store_true',help='Run bounded classic MOD export with native Fast allocator')
    group.add_argument('--project-stream',action='store_true',help='Run streamed master project save with native Fast allocator')
    group.add_argument('--sample-svx',action='store_true',help='Run streamed master IFF export with native Fast allocator')
    group.add_argument('--sample-raw',action='store_true',help='Run streamed master RAW export with native Fast allocator')
    group.add_argument('--sample-wav',action='store_true',help='Run streamed master WAV export with native Fast allocator')
    group.add_argument('--amigus-discovery',action='store_true',help='Discovery-only native library probe; no reservation or MMIO')
    group.add_argument('--studio-memory',choices=['native-abi','mixer','sampler','song','editor','queued','consumer','fifo','session','register-session','reserved-session','sample-ram','wavetable-cache','sampler-wavetable','wavetable-voices','wavetable-dispatch','editor-wavetable','render-sequence'],help='Run one production-allocator Studio fixture')
    group.add_argument('--input-memory',choices=['import','recent','exec-import','exec-recent'],help='Run one import or recent-file memory fixture')
    group.add_argument('--exec-memory',choices=['bounce','stems','failures','save'],help='Run one native Exec-backed memory fixture')
    group.add_argument('--stems-only',action='store_true',help='Run the allocated stem export test only')
    group.add_argument('--wav-only',action='store_true',help='Run the allocated WAV export test only')
    group.add_argument('--memory-failures-only',action='store_true',help='Run all WAV/stem allocation-failure checks only')
    group.add_argument('--bounce-only',action='store_true',help='Run the sample-bounce allocation regression only')
    group.add_argument('--allocated-only',action='store_true',help='Run the three allocated-path tests instead of the two legacy file tests')
    parser.add_argument('--fixture',type=Path,help='Explicit MOD for the EFx stem CLI window')
    parser.add_argument('--candidate',type=Path,help='Manifest-verified PT24GRender for the EFx stem CLI window')
    args=parser.parse_args()
    if (args.fixture or args.candidate) and not args.invert_stem_cli:parser.error('fixture/candidate require --invert-stem-cli')
    if args.candidate:
        manifest=json.loads((args.candidate.parent/'PT24GRender-build.json').read_text())
        if hashlib.sha256(args.candidate.read_bytes()).hexdigest()!=manifest['binary_sha256']:raise RuntimeError('Candidate differs from build manifest')
    cia_manifest=None
    if args.cia_timing:
        cia_manifest=json.loads((ROOT/'build/dev/cia-diagnostic-build.json').read_text())
        if hashlib.sha256((ROOT/'build/dev/PTExecCiaTimingTest').read_bytes()).hexdigest()!=cia_manifest['binary_sha256']:
            raise RuntimeError('CIA candidate differs from exact build manifest')
    sys.path.insert(0,str(INFRA/'scripts'))
    from shared_guest import Guest
    out=ROOT/'build/dev'/('render-files-'+str(time.time_ns()));out.mkdir()
    result={'passed':False,'scope':'shared030 native render/stem file checks'}
    with (INFRA/'runtime/test.lock').open('a') as lock:
        acquire_shared_lock(lock,out,result)
        guest=Guest(INFRA,out)
        if args.cia_timing or args.paula_memory in ('output','output-diagnostic','engine','prepared-output','transport','wait','wait-latency','wait-priority','paula-wait-priority','paula-boundary'):
            audio=guest.command('GET_AUDIO_STATE')
            (out/'audio-before-staging.json').write_text(json.dumps({'audio':audio},indent=2)+'\n')
            if not all('ch%d_dma=0'%i in audio.split('\t') for i in range(4)):
                raise RuntimeError('Silent output fixture refuses an initially active DMA reader')
        try:run=prepare_run(guest,out)
        except Exception:
            result['prelaunch_refused']=True;result['run_files_staged']=False
            (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(out)
            raise
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
        if args.capture_memory:
            cases=[('capture','PTExecCaptureTest','CAPTURE STAGING PASS:')]
            result['scope']='shared030 synthetic capture staging and undoable publication; no device input'
        if args.capture_session_memory:
            cases=[('capture-session','PTExecCaptureSessionTest','CAPTURE SESSION PASS:')]
            result['scope']='shared030 injected recording lifecycle and confirmed-stop ownership; no device input'
        if args.amigus_capture_memory:
            cases=[('amigus-capture','PTExecAmiGusCaptureTest','AMIGUS CAPTURE PASS:')]
            result['scope']='shared030 injected recording PCM lease and interrupt ownership; no card input'
        if args.editor_capture_memory:
            cases=[('editor-capture','PTExecEditorCaptureTest','EDITOR CAPTURE PASS:')]
            result['scope']='shared030 injected editor recording barrier and master publication; no device input'
        if args.paula_memory:
            cases=[{'cache':('sampler-paula','PTExecSamplerPaulaTest','SAMPLER PAULA PASS:'),
                    'voices':('paula-voices','PTExecPaulaVoicesTest','paula voices tests passed'),
                    'preflight':('paula-preflight','PTExecPaulaPreflightTest','PAULA PREFLIGHT PASS:'),
                    'mixed':('mixed-preflight','PTExecMixedPreflightTest','MIXED PREFLIGHT PASS:'),
                    'mixed-owner':('mixed-owner','PTExecMixedOwnerTest','MIXED OWNER PASS:'),
                    'dispatch':('paula-dispatch','PTExecPaulaDispatchTest','Paula prepared batch ownership OK'),
                    'song':('paula-song','PTExecPaulaSongTest','PAULA SONG PASS:'),
                    'editor':('editor-paula','PTExecEditorPaulaTest','EDITOR PAULA PASS:'),
                    'editor-mixed':('editor-mixed','PTExecEditorMixedTest','EDITOR MIXED PASS:'),
                    'reservation':('paula-reservation','PTExecPaulaReservationTest','PAULA RESERVATION PASS:'),
                    'output':('paula-output','PTExecPaulaOutputTest','PAULA OUTPUT PASS:'),
                    'prepared-output':('prepared-paula','PTExecPreparedPaulaTest','PREPARED PAULA PASS:'),
                    'transport':('paula-transport','PTExecPaulaTransportTest','PAULA TRANSPORT PASS:'),
                    'wait':('paula-wait','PTExecPaulaWaitTest','PAULA WAIT PASS:'),
                    'wait-latency':('wait-latency','PTExecWaitLatencyTest','WAIT LATENCY PASS:'),
                    'wait-priority':('wait-priority','PTExecWaitPriorityTest','WAIT LATENCY PASS:'),
                    'paula-wait-priority':('paula-wait-priority','PTExecPaulaWaitPriorityTest','PAULA WAIT PASS:'),
                    'paula-boundary':('paula-boundary','PTExecPaulaBoundaryTest','PAULA WAIT PASS:'),
                    'engine':('paula-engine','PTExecPaulaEngineTest','PAULA ENGINE PASS:'),
                    'output-diagnostic':('paula-output-diagnostic','PTExecPaulaOutputDiagnostic','PAULA OUTPUT DIAGNOSTIC PASS:')}[args.paula_memory]]
            result['scope']='shared030 routed Paula '+args.paula_memory+' with native allocators and injected callbacks; no native DMA/output'
            if args.paula_memory in ('output','output-diagnostic','engine','prepared-output','transport','wait','wait-latency','wait-priority','paula-wait-priority','paula-boundary'):
                result['scope']='shared030 actual audio.device acknowledged silent WRITE/DMA, control, confirmed stop and Chip/channel/IO release; no listening/timing/physical claim'
            if args.paula_memory=='transport':
                result['scope']='shared030 private timer/editor/device lifetime, cancellation and deliberately late startup refusal; zero24 master; no WRITE/cadence/frontend/listening/physical acceptance'
            if args.paula_memory in ('wait','paula-wait-priority'):
                result['scope']='shared030 private Wait/termination/retained-cleanup with exact own-priority scope reported before musical start; zero24 master; no WRITE/output/frontend/listening/physical acceptance'
            if args.paula_memory=='paula-boundary':
                result['scope']='shared030 scoped own-task first musical boundary with actual strict clocks and verified zero24 PCM; silent WRITE permitted only if exact gate succeeds; no frontend/listening/physical acceptance'
            if args.paula_memory in ('wait-latency','wait-priority'):
                result['scope']='shared030 isolated private Wait/timer actual-clock latency with exact own-priority scope reported; no song/cache/device/output/frontend/listening/physical acceptance'
            if args.paula_memory=='prepared-output':
                result['scope']='shared030 prepared editor16/24 ownership with actual device WRITE reading verified zero Chip PCM, logical nonzero volume; classic8 zero-leading segment refused before promotion/cache/output; Fast masters preserved, unused masters unpromoted; numerical time only, no frontend/timer/listening/physical claim'
            if args.paula_memory=='engine':
                result['scope']='shared030 native Fast8/16/24 masters and selective shared Chip cache with four silent audio.device readers; confirmed voice/cache/device closure; no timing/listening/frontend/physical claim'
            if args.paula_memory=='output-diagnostic':
                result['scope']='shared030 one silent device WRITE start-state observation and confirmed cleanup; diagnostic only, not playback acceptance'
            if args.paula_memory=='reservation':
                result['scope']='shared030 actual audio.device channel reservation/lock/free and busy refusal; no WRITE/MMIO/DMA/output'
            if args.paula_memory=='mixed':
                result['scope']='shared030 mixed capability checks on one global sequence; native Fast allocation, no caches/devices/DMA/output'
            if args.paula_memory=='mixed-owner':
                result['scope']='shared030 injected combined master/voice ownership, native Fast/Chip allocations; no mixed scheduler/devices/output'
        if args.cia_timing:
            cases=[('cia-timing','PTExecCiaTimingTest','CIA TIMING PASS:')]
            result['source_tree']=cia_manifest['source_tree']
            result['scope']='shared030 finite owned free CIA timer/IRQ actual-clock admission diagnostic with independent termination; no Paula/audio.device/output/frontend/physical acceptance'
        if args.recovery_file:
            cases=[('recovery','PTExecRecoveryTest','RECOVERY FILE PASS:')]
            result['scope']='shared030 explicit recovery file transactions; no automatic snapshots or UI wiring'
        if args.studio_memory:
            cases=[{'native-abi':('native-abi','PTAmiGusNativeAbiTest','AMIGUS NATIVE ABI PASS:'),
                    'mixer':('studio','PTExecStudioTest','STUDIO MIX PASS:'),
                    'sampler':('sampler-studio','PTExecSamplerStudioTest','SAMPLER STUDIO PASS:'),
                    'render-sequence':('render-sequence','PTExecRenderSequenceTest','SEQUENCE PASS:'),
                    'editor-wavetable':('editor-wavetable','PTExecEditorWavetableTest','EDITOR WAVETABLE PASS:'),
                    'wavetable-dispatch':('wavetable-dispatch','PTExecWavetableDispatchTest','WAVETABLE DISPATCH PASS:'),
                    'wavetable-voices':('wavetable-voices','PTExecWavetableVoicesTest','WAVETABLE VOICES PASS:'),
                    'sampler-wavetable':('sampler-wavetable','PTExecSamplerWavetableTest','SAMPLER WAVETABLE PASS:'),
                    'wavetable-cache':('wavetable-cache','PTExecAmiGusWavetableCacheTest','AMIGUS WAVETABLE OWNER PASS:'),
                    'sample-ram':('sample-ram','PTExecAmiGusSampleRamTest','AMIGUS SAMPLE RAM PASS:'),
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
        if args.stem_cli or args.invert_stem_cli:
            cases=[('stem-cli','PT24GRender','STEMS count=2') ]
            result['scope']='shared030 Fast-allocator stem CLI, exact two-stem host parity and destination refusal'
            if args.invert_stem_cli:result['scope']+='; all-channel shared EFx mutation'
        if args.render_cli:
            cases=[('render-cli','PT24GRender','WAV frames=')]
            result['scope']='shared030 Fast-allocator renderer, one-row24-bit WAV host parity'
        if args.pp20_import:
            cases=[('pp20-import','PT24GConvert','SAVED format=MOD')]
            result['scope']='shared030 bounded PP20 converter load and exact MOD export'
        if args.project_import:
            cases=[('project-import','PT24GConvert','SAVED format=PT24G-v1')]
            result['scope']='shared030 bounded enhanced-project converter load/save; exact mixed master roundtrip'
        if args.sample_dispatch:
            cases=[('sample-dispatch','PTExecSampleDispatchTest','SAMPLE DISPATCH PASS:')]
            result['scope']='shared030 bounded sample dispatch and retained24-bit masters'
        if args.source_memory:
            cases=[('source-memory','PTExecSourceTest','MOD SOURCE PASS:')]
            result['scope']='shared030 donor ownership and failure checks with Exec Fast allocator'
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
        if args.invert_cli:
            cases=[('invert-cli','PT24GRender','WAV frames=')]
            result['scope']='shared030 bounded EFx CLI exact host WAV; no physical/audio acceptance'
        if args.invert_bounce:
            cases=[('invert-bounce','PTExecInvertBounceTest','INVERT BOUNCE PASS:')]
            result['scope']='shared030 EFx bounce and production Fast allocator; no physical acceptance'
        if args.invert_render:
            cases=[('invert-render','PTInvertRenderTest','INVERT render PASS:')]
            result['scope']='shared030 offline EFx exact PCM and resource checks; no audio/physical acceptance'
        if args.invert_session:
            cases=[('invert-session','PTExecInvertSessionTest','INVERT QUEUED PASS:')]
            result['scope']='shared030 private queued EFx PCM and Fast ownership; no device/physical acceptance'
        if args.invert_sampler:
            cases=[('invert-sampler','PTExecSamplerInvertSongTest','SAMPLER INVERT SONG PASS:')]
            result['scope']='shared030 sampler EFx version/private lifetime and Fast ownership; no device/physical acceptance'
        if args.invert_editor:
            cases=[('invert-editor','PTExecEditorInvertStudioTest','EDITOR INVERT STUDIO PASS:')]
            result['scope']='shared030 editor EFx ownership and Fast allocator; no native UI/device/physical acceptance'
        try:
            commands=['FailAt 21','Stack 65536']
            for name,binary,marker in cases:
                sub=run/name;sub.mkdir();shutil.copyfile(args.candidate if args.candidate else ROOT/'build/dev'/binary,sub/binary)
                result[binary+'_sha256']=hashlib.sha256((sub/binary).read_bytes()).hexdigest()
                if args.cia_timing and result[binary+'_sha256']!=cia_manifest['binary_sha256']:
                    raise RuntimeError('Staged CIA binary differs from coordinated manifest')
                commands+=['CD '+guest.device+run.name+'/'+name,binary+' '+guest.device+run.name+'/'+name+('/sample.input' if args.sample_dispatch else '/donor.mod' if args.source_memory else '/module.mod' if args.mod_import else '/sample.input' if args.sample_import else '/master.mod' if args.mod_stream else '/master.ptg' if args.project_stream else '/sample.iff' if args.sample_svx else '/sample.raw' if args.sample_raw else '/sample.wav' if args.sample_wav else '/recent' if args.input_memory in ('recent','exec-recent') else '')+' >test.log','Echo $RC >test.rc']
            if args.recovery_file:
                directory=guest.device+run.name+'/recovery'
                shutil.copyfile(ROOT/'tests/fixtures/project-v1/mixed.ptg',run/'recovery'/'source.ptg')
                commands=['FailAt 21','Stack 65536','CD '+directory,
                          'PTExecRecoveryTest '+directory+'/source.ptg '+directory+' >test.log','Echo $RC >test.rc']
            if args.invert_render:
                commands=['FailAt 21','Stack 65536',guest.device+run.name+'/invert-render/PTInvertRenderTest >'+guest.device+run.name+'/invert-render/test.log','Echo $RC >'+guest.device+run.name+'/invert-render/test.rc']
            if args.source_memory:
                from make_mod_sample_fixture import make
                donor=make((ROOT/'evidence/baseline/mod.baseline').read_bytes())
                (run/'source-memory/donor.mod').write_bytes(donor)
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
            if args.invert_cli:
                sub=run/'invert-cli';shutil.copyfile(ROOT/'evidence/enhanced-editor/invert-ordering/invert_delay.mod',sub/'source.mod')
                commands=['FailAt 21','Stack 65536','CD '+guest.device+run.name+'/invert-cli',
                          'PT24GRender source.mod output.wav --invert-budget 100000 --gain 65536 >test.log','Echo $RC >test.rc']
            if args.render_cli:
                sub=run/'render-cli';shutil.copyfile(ROOT/'evidence/baseline/mod.baseline',sub/'source.mod')
                commands=['FailAt 21','Stack 65536','CD '+guest.device+run.name+'/render-cli',
                          'PT24GRender source.mod output.wav --pattern 0 --from-row 0 --to-row 1 --tracks 1 >test.log','Echo $RC >test.rc']
            if args.stem_cli or args.invert_stem_cli:
                sub=run/'stem-cli';fixture=args.fixture or ROOT/('evidence/enhanced-editor/invert-ordering/invert_shared.mod' if args.invert_stem_cli else 'evidence/baseline/mod.baseline')
                shutil.copyfile(fixture,sub/'source.mod');result['fixture_sha256']=hashlib.sha256(fixture.read_bytes()).hexdigest()
                command='PT24GRender source.mod stems --pattern 0 --from-row 0 --to-row 1 --tracks 3 --stems'
                if args.invert_stem_cli:command='PT24GRender source.mod stems --tracks 3 --stems --invert-budget 100000 --gain 65536'
                commands=['FailAt 21','Stack 65536','CD '+guest.device+run.name+'/stem-cli',
                          command+' >test.log','Echo $RC >test.rc',command+' >repeat.log','Echo $RC >repeat.rc']
            # Release the guest current-directory lock before host cleanup.
            commands+=['FailAt 1','CD RAM:','Echo done >'+guest.device+run.name+'/done']
            if args.recovery_file:
                # The functional test may start only after the separate negative
                # probe has exited normally and returned every owned allocation.
                probe=['FailAt 21','Stack 65536','CD '+directory,
                       'PTExecRecoveryTest --failure-cleanup >failure.log','Echo $RC >failure.rc',
                       'FailAt 1','CD RAM:','Echo done >'+guest.device+run.name+'/probe-done']
                require_running_guest(guest,out,'before-probe')
                guest.launch.write_text('\n'.join(probe)+'\n');guest.start()
                deadline=time.monotonic()+20
                while not (run/'probe-done').exists():
                    if time.monotonic()>deadline:raise RuntimeError('Cleanup probe timed out; functional test not started')
                    time.sleep(.1)
                failure=(run/'recovery/failure.log').read_text()
                (out/'intentional-failure.log').write_text(failure)
                rc=(run/'recovery/failure.rc').read_text().strip()
                if rc!='20' or 'intentional cleanup probe' not in failure or 'EXEC MEMORY FAILURE CLEANUP: zero owned bytes' not in failure:
                    raise RuntimeError('Cleanup probe failed; functional test not started')
                result['failure_cleanup_verified_before_functional_launch']=True
                state=guest.command('GET_AUDIO_STATE')
                if not all('ch%d_dma=0'%i in state.split('\t') for i in range(4)):
                    raise RuntimeError('Cleanup probe left active or unknown DMA; functional test not started')
            require_running_guest(guest,out,'before-launch')
            if args.cia_timing or args.paula_memory in ('output','output-diagnostic','engine','prepared-output','transport','wait','wait-latency','wait-priority','paula-wait-priority','paula-boundary'):
                audio=guest.command('GET_AUDIO_STATE')
                (out/'audio-before-launch.json').write_text(json.dumps({'audio':audio},indent=2)+'\n')
                if not all('ch%d_dma=0'%i in audio.split('\t') for i in range(4)):
                    raise RuntimeError('Silent output fixture refuses DMA active before launch')
            guest.launch.write_text('\n'.join(commands)+'\n');guest.start()
            # The cumulative editor-wavetable fixture includes native timer and
            # cancellation diagnostics; its process budget is not an audio deadline.
            deadline=time.monotonic()+(120 if args.studio_memory=='editor-wavetable' or args.invert_editor or args.invert_session or args.invert_sampler else 60 if args.invert_render or args.invert_bounce else 240 if args.invert_stem_cli else 180 if args.invert_cli else 90)
            while not (run/'done').exists():
                if time.monotonic()>deadline:raise RuntimeError('Native file checks timed out; preserve owned run for recovery')
                time.sleep(.2)
            finished=True
            for name,binary,marker in cases:
                log=(run/name/'test.log').read_text();(out/(name+'.log')).write_text(log)
                result[name+'_returncode']=(run/name/'test.rc').read_text().strip()
                assert result[name+'_returncode']=='0' and marker in log,log
                if args.capture_memory or args.capture_session_memory or args.amigus_capture_memory or args.editor_capture_memory or args.recovery_file or args.invert_editor or args.invert_sampler or args.invert_session or args.invert_bounce or args.sample_dispatch or args.source_memory or args.mod_import or args.sample_import or args.mod_stream or args.project_stream or args.sample_svx or args.sample_raw or args.sample_wav or args.studio_memory or args.exec_memory or args.input_memory in ('exec-import','exec-recent'):assert 'EXEC MEMORY PASS:' in log,log
            if args.mod_import or args.sample_import or args.mod_stream or args.project_stream or args.sample_svx or args.sample_wav or args.sample_raw:
                directory,binary=('mod-import','PTExecModImportTest') if args.mod_import else ('svx-import','PTExecSvxImportTest') if args.sample_import=='svx' else ('raw-import','PTExecRawImportTest') if args.sample_import=='raw' else ('wav-import','PTExecWavImportTest') if args.sample_import=='wav' else ('mod-stream','PTExecModStreamTest') if args.mod_stream else ('project-stream','PTExecProjectStreamTest') if args.project_stream else ('sample-svx','PTExecSampleSvxFileTest') if args.sample_svx else ('sample-raw','PTExecSampleRawFileTest') if args.sample_raw else ('sample-wav','PTExecSampleFileTest')
                remaining=sorted(p.name for p in (run/directory).iterdir())
                assert remaining==[binary,'test.log','test.rc'],remaining
                result['sample_staging_clean']=True
            if args.recovery_file:
                sub=run/'recovery';expected=(ROOT/'tests/fixtures/project-v1/mixed.ptg').read_bytes()
                assert (sub/'source.ptg').read_bytes()==expected
                assert sorted(p.name for p in sub.iterdir())==['PTExecRecoveryTest','failure.log','failure.rc','source.ptg','test.log','test.rc']
                failure=(sub/'failure.log').read_text();failure_rc=(sub/'failure.rc').read_text().strip()
                (out/'intentional-failure.log').write_text(failure)
                assert failure_rc=='20' and 'intentional cleanup probe' in failure and 'EXEC MEMORY FAILURE CLEANUP: zero owned bytes' in failure
                result['intentional_failure_returncode']=failure_rc
                result['failure_cleanup_verified']=True
                result['source_unchanged']=True
                result['fixture_sha256']=hashlib.sha256(expected).hexdigest()
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
            if args.invert_cli:
                sub=run/'invert-cli';expected=Path(args.invert_cli).read_bytes()
                assert (sub/'output.wav').read_bytes()==expected
                assert (sub/'source.mod').read_bytes()==(ROOT/'evidence/enhanced-editor/invert-ordering/invert_delay.mod').read_bytes()
                assert sorted(p.name for p in sub.iterdir())==['PT24GRender','output.wav','source.mod','test.log','test.rc']
                result['exact_host_wav']=True;result['reference_sha256']=hashlib.sha256(expected).hexdigest()
            if args.render_cli:
                sub=run/'render-cli';expected=Path(args.render_cli).read_bytes()
                assert (sub/'output.wav').read_bytes()==expected
                assert (sub/'source.mod').read_bytes()==(ROOT/'evidence/baseline/mod.baseline').read_bytes()
                assert sorted(p.name for p in sub.iterdir())==['PT24GRender','output.wav','source.mod','test.log','test.rc']
                result['exact_host_wav']=True
                result['reference_sha256']=hashlib.sha256(expected).hexdigest()
            if args.stem_cli or args.invert_stem_cli:
                sub=run/'stem-cli';reference=Path(args.invert_stem_cli or args.stem_cli)
                expected={p.name:p.read_bytes() for p in reference.iterdir()}
                assert sorted(expected)==['track-01.wav','track-02.wav']
                assert {p.name:p.read_bytes() for p in (sub/'stems').iterdir()}==expected
                assert (sub/'source.mod').read_bytes()==fixture.read_bytes()
                repeat=(sub/'repeat.rc').read_text().strip();assert repeat=='20',repeat
                assert sorted(p.name for p in sub.iterdir())==['PT24GRender','repeat.log','repeat.rc','source.mod','stems','test.log','test.rc']
                (out/'repeat.log').write_text((sub/'repeat.log').read_text())
                result['repeat_returncode']=repeat
                result['exact_host_stems']=True
                result['reference_sha256']={n:hashlib.sha256(data).hexdigest() for n,data in expected.items()}
            if args.sample_dispatch:
                assert not (run/'sample-dispatch/sample.input').exists()
                result['sample_staging_clean']=True
            if args.source_memory:
                assert (run/'source-memory/donor.mod').read_bytes()==donor
                state=guest.command('GET_AUDIO_STATE')
                assert all('ch%d_dma=0'%i in state.split('\t') for i in range(4))
                result['stopped_audio']=state
                result['donor_unchanged']=True
            result['passed']=True
        finally:
            finish_run(guest,run,out,result,finished,
                (args.cia_timing or args.paula_memory or args.capture_memory or args.capture_session_memory or args.amigus_capture_memory or args.editor_capture_memory or args.recovery_file or args.studio_memory in ('native-abi','editor','sample-ram','wavetable-cache','sampler-wavetable','wavetable-voices','wavetable-dispatch','editor-wavetable','render-sequence') or args.invert_editor or args.invert_sampler or args.invert_session or args.source_memory or args.sample_dispatch or args.invert_render or args.invert_bounce or args.invert_cli or args.invert_stem_cli))
if __name__=='__main__':main()
