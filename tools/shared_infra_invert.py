#!/usr/bin/env python3
"""Capture extended EFx ordering on an explicitly reserved shared030 guest."""
import argparse,fcntl,hashlib,json,shutil,sys,time
from pathlib import Path
from make_invert_fixtures import extended_fixtures,one_shot_fixtures,handoff_fixtures,handoff_command_fixtures
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

def canonical_handoff(trace,data):
    """Normalize each independently allocated sample, preserving pointer offsets.

    Restricted fixture contract: one pattern, channel0 instrument-only changes,
    no offsets, at most31 samples. Raw captures are always retained separately.
    """
    assert len(trace)%164==0 and data[950]==1 and data[952]==0
    offsets=[];sizes=[];offset=2108
    for i in range(31):
        size=int.from_bytes(data[42+30*i:44+30*i],'big')*2
        offsets.append(offset);sizes.append(size);offset+=size
    assert offset==len(data)
    bases={};instrument=0
    for start in range(0,len(trace),164):
        r=trace[start:start+164];pointer=int.from_bytes(r[52:56],'big')
        if pointer==0xffffffff:continue
        row=int.from_bytes(r[6:8],'big')//16;assert row<64
        event=data[1084+row*16:1088+row*16]
        number=(event[0]&240)|(event[2]>>4)
        if number:instrument=number-1
        assert sizes[instrument]
        if instrument in bases:assert bases[instrument]==pointer
        else:bases[instrument]=pointer
    assert len(bases)==2
    out=bytearray(trace)
    for start in range(0,len(trace),164):
        for field in [52+22*ch+f for ch in range(4) for f in (0,6,14)]+[140]:
            pos=start+field;pointer=int.from_bytes(trace[pos:pos+4],'big')
            if pointer==0xffffffff:continue
            matches=[(i,(pointer-base)&0xffffffff) for i,base in bases.items() if ((pointer-base)&0xffffffff)<sizes[i]]
            assert len(matches)==1, (field,pointer,matches)
            i,delta=matches[0];out[pos:pos+4]=(offsets[i]+delta).to_bytes(4,'big')
    return bytes(out)

def canonical_commands(trace,data):
    """Map the two fixture allocations from stable loop pointers, not9xx n_start.

    One pattern/order and channel0 only. Derive each base from the MOD loop
    offset, assert it is stable on every tick, then relocate every recorded
    address by that same base. Offset/retrigger addresses remain observable.
    """
    assert len(trace)%164==0 and data[950]==1 and data[952]==0
    offsets=[];sizes=[];loops=[];offset=2108
    for i in range(31):
        size=int.from_bytes(data[42+30*i:44+30*i],'big')*2
        offsets.append(offset);sizes.append(size);offset+=size
        loops.append(int.from_bytes(data[46+30*i:48+30*i],'big')*2)
    assert offset==len(data)
    bases={};instrument=0
    for start in range(0,len(trace),164):
        r=trace[start:start+164];pointer=int.from_bytes(r[58:62],'big')
        if pointer==0xffffffff:continue
        row=int.from_bytes(r[6:8],'big')//16;assert row<64
        event=data[1084+row*16:1088+row*16]
        number=(event[0]&240)|(event[2]>>4)
        if number:instrument=number-1
        assert sizes[instrument]
        base=(pointer-loops[instrument])&0xffffffff
        if instrument in bases:assert bases[instrument]==base
        else:bases[instrument]=base
    assert len(bases)==2
    out=bytearray(trace)
    for start in range(0,len(trace),164):
        for field in [52+22*ch+f for ch in range(4) for f in (0,6,14)]+[140]:
            pos=start+field;pointer=int.from_bytes(trace[pos:pos+4],'big')
            if pointer==0xffffffff:continue
            matches=[(i,(pointer-base)&0xffffffff) for i,base in bases.items() if ((pointer-base)&0xffffffff)<sizes[i]]
            assert len(matches)==1,(field,pointer,matches)
            i,delta=matches[0];out[pos:pos+4]=(offsets[i]+delta).to_bytes(4,'big')
    return bytes(out)

def main():
    parser=argparse.ArgumentParser(description=__doc__);group=parser.add_mutually_exclusive_group();group.add_argument("--one-shot",action="store_true");group.add_argument("--handoff",action="store_true");group.add_argument("--handoff-commands",action="store_true");args=parser.parse_args()
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
            cases=list(handoff_command_fixtures() if args.handoff_commands else handoff_fixtures() if args.handoff else one_shot_fixtures() if args.one_shot else extended_fixtures());commands=['FailAt 1','Stack 65536','CD '+guest.device+run.name]
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
                canonical=lambda trace:canonical_commands(trace,data) if args.handoff_commands else canonical_handoff(trace,data) if args.handoff else canonical_trace(trace)
                assert canonical(captures[0][0])==canonical(captures[1][0]), name
                (out/(name+".trace")).write_bytes(canonical(captures[0][0]))
                trace,reason=captures[0];(out/(name+'.mod')).write_bytes(data)
                report['cases'][name]={**meta,'comparison':('exact after per-sample cache relocation' if (args.handoff or args.handoff_commands) else 'exact after one fixed sample-cache relocation')+'; raw logs retained','reason':reason,'ticks':len(trace)//164,'fixture_sha256':hashlib.sha256(data).hexdigest(),'trace_sha256':hashlib.sha256(trace).hexdigest()}
            report['passed']=True
        finally:
            if (run/'done').exists():
                state=guest.command('GET_AUDIO_STATE');report['audio']=state
                finished=all('ch%d_dma=0'%i in state.split('\t') for i in range(4))
                if finished:guest.launch.unlink();shutil.rmtree(run)
            report['owned_files_cleaned']=finished
            (out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(out)
if __name__=='__main__':main()
