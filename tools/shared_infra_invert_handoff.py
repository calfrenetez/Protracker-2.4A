#!/usr/bin/env python3
"""Verify captured EFx handoff PCM on an explicitly reserved shared030 guest.

Reserve a420-second window: the one-frame pull oracle is deliberately expensive.
"""
import argparse,fcntl,hashlib,json,shutil,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
def main():
    parser=argparse.ArgumentParser(description=__doc__);group=parser.add_mutually_exclusive_group();group.add_argument("--commands",action="store_true");group.add_argument("--shared",action="store_true");args=parser.parse_args()
    sys.path.insert(0,str(INFRA/'scripts'))
    from shared_guest import Guest
    evidence=ROOT/'evidence/enhanced-editor'/('invert-shared-handoff' if args.shared else 'invert-handoff-commands' if args.commands else 'invert-handoff')
    cases=sorted(p.stem for p in evidence.glob('*.mod'));assert len(cases)==(3 if args.shared else 6 if args.commands else 4)
    out=ROOT/'build/dev'/('invert-handoff-'+str(time.time_ns()));out.mkdir()
    report={'passed':False,'scope':'shared030 reference-driven EFx PCM core; no device/audio/physical acceptance'}
    with (INFRA/'runtime/test.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        guest=Guest(INFRA,out);run=guest.share/out.name;run.mkdir()
        try:
            binary=run/('PTExecSharedInvertHandoffTest' if args.shared else 'PTExecInvertHandoffTest');shutil.copyfile(ROOT/'build/dev'/binary.name,binary)
            report['binary_sha256']=hashlib.sha256(binary.read_bytes()).hexdigest()
            commands=['FailAt 1','Stack 65536','CD '+guest.device+run.name]
            for name in cases:
                for suffix in ('.mod','.trace'):shutil.copyfile(evidence/(name+suffix),run/(name+suffix))
                commands+=[binary.name+' '+name+'.mod '+name+'.trace >'+name+'.log','Echo $RC >'+name+'.rc']
            commands+=['Echo done >done'];guest.launch.write_text('\n'.join(commands)+'\n');guest.start()
            deadline=time.monotonic()+420
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
                assert rc=='0' and 'EFx handoff reference PCM PASS:' in log and 'EXEC MEMORY PASS:' in log
                report['cases'][name]={'rc':0,'log_sha256':hashlib.sha256(log.encode()).hexdigest()}
            report['passed']=True
        finally:
            report['owned_files_cleaned']=False
            if (run/'done').exists():
                state=guest.command('GET_AUDIO_STATE');report['audio']=state
                if all('ch%d_dma=0'%i in state.split('\t') for i in range(4)):
                    guest.launch.unlink();shutil.rmtree(run)
                    report['owned_files_cleaned']=not run.exists() and not guest.launch.exists()
            (out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(out)
if __name__=='__main__':main()
