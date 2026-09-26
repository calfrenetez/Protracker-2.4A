#!/usr/bin/env python3
"""Capture extended EFx ordering on an explicitly reserved shared030 guest."""
import argparse,fcntl,hashlib,json,shutil,sys,time
from pathlib import Path
from make_invert_fixtures import extended_fixtures,one_shot_fixtures
from test_invert_emulator import decode_trace
ROOT=Path(__file__).resolve().parents[1]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
def canonical_trace(trace):
    """Remove one allocation relocation for these sample-1-only fixtures.

    Keep every byte except the 13 explicitly recorded pointers. One fixed
    delta for the entire capture preserves cursor motion, pointer relationships,
    sentinel values and all sample mutations; this is not a general MOD mapper.
    """
    assert len(trace)%164==0
    pointer_offsets=[52+22*ch+field for ch in range(4) for field in (0,6,14)]+[140]
    starts=[int.from_bytes(trace[i+52:i+56],'big') for i in range(0,len(trace),164)]
    active=[v for v in starts if v!=0xffffffff]
    assert active and len(set(active))==1, 'fixture requires one unchanged sample start'
    delta=active[0]-2108
    result=bytearray(trace)
    for record in range(0,len(trace),164):
        for field in pointer_offsets:
            pos=record+field;value=int.from_bytes(trace[pos:pos+4],'big')
            if value!=0xffffffff:
                result[pos:pos+4]=((value-delta)&0xffffffff).to_bytes(4,'big')
    return bytes(result)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--one-shot",action="store_true");args=parser.parse_args()
    sys.path.insert(0,str(INFRA/'scripts'))
    from shared_guest import Guest
    out=ROOT/'build/dev'/('invert-shared-'+str(time.time_ns()));out.mkdir()
    report={'passed':False,'scope':'pinned EFx ordering captures; no renderer/physical acceptance'}
    finished=False
    with (INFRA/'runtime/test.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        guest=Guest(INFRA,out);run=guest.share/out.name;run.mkdir()
        try:
            binary=run/'PTInvertTraceTest';shutil.copyfile(ROOT/'build/dev/PTInvertTraceTest',binary)
            report['diagnostic_sha256']=hashlib.sha256(binary.read_bytes()).hexdigest()
            cases=list(one_shot_fixtures() if args.one_shot else extended_fixtures());commands=['FailAt 1','Stack 65536','CD '+guest.device+run.name]
            for name,data,meta in cases:
                (run/(name+'.mod')).write_bytes(data)
                for repeat in range(2):
                    stem=name+str(repeat)
                    commands += ['PTInvertTraceTest '+name+'.mod '+str(meta['max_ticks'])+' >'+stem+'.log','Echo $RC >'+stem+'.rc']
            commands+=['Echo done >done'];guest.launch.write_text('\n'.join(commands)+'\n');guest.start()
            deadline=time.monotonic()+90
            while not (run/'done').exists():
                if time.monotonic()>deadline:raise RuntimeError('Capture timeout; retain owned run for recovery')
                time.sleep(.2)
            # Preserve every raw artifact before comparisons can fail.
            for name,data,meta in cases:
                (out/(name+'.mod')).write_bytes(data)
                for repeat in range(2):
                    for suffix in ('.log','.rc'):
                        source=run/(name+str(repeat)+suffix)
                        if source.exists():shutil.copyfile(source,out/source.name)
            report['cases']={}
            for name,data,meta in cases:
                captures=[]
                for repeat in range(2):
                    stem=name+str(repeat);log=(run/(stem+'.log')).read_text();(out/(stem+'.log')).write_text(log)
                    assert (run/(stem+'.rc')).read_text().strip()=='0'
                    captures.append(decode_trace(log,meta['max_ticks']))
                assert captures[0][1]==captures[1][1]
                assert canonical_trace(captures[0][0])==canonical_trace(captures[1][0]), name
                trace,reason=captures[0];(out/(name+'.mod')).write_bytes(data)
                report['cases'][name]={**meta,'comparison':'exact after one fixed sample-cache relocation; raw logs retained','reason':reason,'ticks':len(trace)//164,'fixture_sha256':hashlib.sha256(data).hexdigest(),'trace_sha256':hashlib.sha256(trace).hexdigest()}
            report['passed']=True
        finally:
            if (run/'done').exists():
                state=guest.command('GET_AUDIO_STATE');report['audio']=state
                finished=all('ch%d_dma=0'%i in state.split('\t') for i in range(4))
                if finished:guest.launch.unlink();shutil.rmtree(run)
            report['owned_files_cleaned']=finished
            (out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(out)
if __name__=='__main__':main()
