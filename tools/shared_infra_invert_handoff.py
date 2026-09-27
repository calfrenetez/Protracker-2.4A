#!/usr/bin/env python3
"""Verify captured EFx handoff PCM on an explicitly reserved shared030 guest.

Reserve420 seconds for existing batches; --delay requires one --case and a
180-second window, or240 seconds with --editor for queue/edit ownership.
The longer-loop native oracle uses17/256-frame pulls.
"""
import argparse,fcntl,hashlib,json,shutil,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
def main():
    parser=argparse.ArgumentParser(description=__doc__);group=parser.add_mutually_exclusive_group();group.add_argument("--commands",action="store_true");group.add_argument("--shared",action="store_true");group.add_argument("--writes",action="store_true");group.add_argument("--delay",action="store_true");parser.add_argument("--case");parser.add_argument("--editor",action="store_true");args=parser.parse_args()
    sys.path.insert(0,str(INFRA/'scripts'))
    from shared_guest import Guest
    evidence=ROOT/'evidence/enhanced-editor'/('invert-four-clock-delay' if args.delay else 'invert-write-events' if args.writes else 'invert-shared-handoff' if args.shared else 'invert-handoff-commands' if args.commands else 'invert-handoff')
    cases=sorted(p.stem for p in evidence.glob('*.mod'));assert len(cases)==(2 if args.delay else 3 if args.shared else 6 if args.commands else 4)
    if args.editor and not args.delay:parser.error('--editor requires --delay')
    if args.delay and not args.case:parser.error('--delay requires one --case for a bounded window')
    if args.case:
        if args.case not in cases:parser.error('unknown fixture case')
        cases=[args.case]
    metadata=json.loads((evidence/'result.json').read_text()) if args.delay else None
    seconds=240 if args.editor else 180 if args.delay else 420
    out=ROOT/'build/dev'/('invert-handoff-'+str(time.time_ns()));out.mkdir()
    started=time.monotonic()
    report={'passed':False,'scope':'shared030 reference-driven EFx PCM core; no device/audio/physical acceptance','deadline_seconds':seconds}
    with (INFRA/'runtime/test.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        guest=Guest(INFRA,out);run=guest.share/out.name;run.mkdir()
        try:
            binary=run/('PTExecEditorInvertReferenceTest' if args.editor else 'PTExecInvertWriteTest' if (args.writes or args.delay) else 'PTExecSharedInvertHandoffTest' if args.shared else 'PTExecInvertHandoffTest');shutil.copyfile(ROOT/'build/dev'/binary.name,binary)
            report['binary_sha256']=hashlib.sha256(binary.read_bytes()).hexdigest()
            commands=['FailAt 1','Stack 65536','CD '+guest.device+run.name]
            for name in cases:
                for suffix in ('.mod','.trace'):shutil.copyfile(evidence/(name+suffix),run/(name+suffix))
                ticks=' '+str(metadata['cases'][name]['active_ticks']) if args.delay else ''
                commands+=[binary.name+' '+name+'.mod '+name+'.trace'+ticks+' >'+name+'.log','Echo $RC >'+name+'.rc']
            commands+=['Echo done >done'];guest.launch.write_text('\n'.join(commands)+'\n');guest.start()
            deadline=time.monotonic()+seconds
            while not (run/'done').exists():
                if time.monotonic()>deadline:raise RuntimeError('Timeout: retain exact run for coordinated recovery; no retry')
                time.sleep(.2)
            for name in cases:
                for suffix in ('.log','.rc'):
                    source=run/(name+suffix)
                    if source.exists():shutil.copyfile(source,out/source.name)
            report['cases']={}
            for name in cases:
                log=(out/(name+'.log')).read_text();rc=(out/(name+'.rc')).read_text().strip()
                marker='EDITOR EFx REFERENCE PASS:' if args.editor else 'EFx handoff reference PCM PASS:'
                assert rc=='0' and marker in log and 'EXEC MEMORY PASS:' in log
                report['cases'][name]={'rc':0,'log_sha256':hashlib.sha256(log.encode()).hexdigest()}
            report['passed']=True
        finally:
            report['owned_files_cleaned']=False
            if (run/'done').exists():
                state=guest.command('GET_AUDIO_STATE');report['audio']=state
                if all('ch%d_dma=0'%i in state.split('\t') for i in range(4)):
                    guest.launch.unlink();shutil.rmtree(run)
                    report['owned_files_cleaned']=not run.exists() and not guest.launch.exists()
            report['elapsed_seconds']=time.monotonic()-started
            (out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(out)
if __name__=='__main__':main()
