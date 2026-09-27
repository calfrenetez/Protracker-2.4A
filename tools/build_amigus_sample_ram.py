#!/usr/bin/env python3
"""Build bounded sample RAM/cache fixture; injected bus only, no native card I/O."""
import argparse,ast,json,subprocess
from pathlib import Path
from build_diagnostic import ROOT,digest,compiler_safety_flags,runtime_inputs

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cc',required=True);group=parser.add_mutually_exclusive_group();group.add_argument('--owner',action='store_true');group.add_argument('--sampler',action='store_true');group.add_argument('--voices',action='store_true');group.add_argument('--dispatch',action='store_true');args=parser.parse_args()
    inputs=['tests/native_exec_amigus_sample_ram_test.c',*['src/core/'+n+'.c' for n in ['amigus_sample_ram','sample_cache','playback_pcm','pcm']]]
    if args.owner:
        inputs[0]='tests/native_exec_amigus_wavetable_cache_test.c'
        inputs+=['src/core/amigus_reservation.c','src/core/amigus_wavetable_cache.c','src/native/amigus_reservation.c']
        lock=json.loads((ROOT/'amigus-sdk.lock.json').read_text())
        for name,expected in lock['files'].items():
            if digest(ROOT/'vendor/amigus-sdk'/name)!=expected:raise RuntimeError('SDK changed: '+name)
    if args.sampler or args.voices or args.dispatch:
        inputs[0]='tests/native_exec_sampler_wavetable_test.c'
        inputs+=['src/core/amigus_reservation.c','src/core/amigus_wavetable_cache.c','src/editor/sampler_wavetable.c']
        tree=ast.parse((ROOT/'tests/test_sampler.py').read_text())
        sources=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='SOURCES' for t in n.targets))
        inputs=list(dict.fromkeys(inputs+sources[1:]))
        if args.voices or args.dispatch:
            inputs[0]='tests/native_exec_wavetable_voices_test.c'
            inputs+=['src/editor/wavetable_voices.c','src/core/amigus_voice_plan.c']
    if args.dispatch:
        inputs[0]='tests/native_exec_wavetable_dispatch_test.c'
        tree=ast.parse((ROOT/'tests/test_wavetable_dispatch.py').read_text())
        sources=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='DISPATCH' for t in n.targets))
        inputs=list(dict.fromkeys(inputs+sources))
    flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core','-Ivendor/amigus-sdk',*compiler_safety_flags(args.cc)]
    out=ROOT/'build/dev'/('PTExecWavetableDispatchTest' if args.dispatch else 'PTExecWavetableVoicesTest' if args.voices else 'PTExecSamplerWavetableTest' if args.sampler else 'PTExecAmiGusWavetableCacheTest' if args.owner else 'PTExecAmiGusSampleRamTest');out.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run([args.cc,*flags,*inputs,'-o',str(out)],cwd=ROOT,check=True)
    dependencies=set()
    for source in inputs:
        raw=subprocess.check_output([args.cc,*flags,'-MM',source],cwd=ROOT,text=True).replace('\\\n',' ')
        dependencies.update(str((ROOT/p).resolve().relative_to(ROOT)) for p in raw.split(':',1)[1].split())
    report=dict(scope='compile/link; injected bus, no card access',flags=flags,inputs={p:digest(ROOT/p) for p in sorted(dependencies)},compiler_sha256=digest(Path(args.cc)),runtime_inputs=runtime_inputs(args.cc),binary_sha256=digest(out),binary_bytes=out.stat().st_size)
    out.with_name('wavetable-dispatch-build.json' if args.dispatch else 'wavetable-voices-build.json' if args.voices else 'sampler-wavetable-build.json' if args.sampler else 'amigus-wavetable-cache-build.json' if args.owner else 'amigus-sample-ram-build.json').write_text(json.dumps(report,indent=2)+'\n');print(out)
if __name__=='__main__':main()
