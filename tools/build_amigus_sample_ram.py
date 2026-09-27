#!/usr/bin/env python3
"""Build bounded sample RAM/cache fixture; injected bus only, no native card I/O."""
import argparse,json,subprocess
from pathlib import Path
from build_diagnostic import ROOT,digest,compiler_safety_flags,runtime_inputs

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cc',required=True);parser.add_argument('--owner',action='store_true');args=parser.parse_args()
    inputs=['tests/native_exec_amigus_sample_ram_test.c',*['src/core/'+n+'.c' for n in ['amigus_sample_ram','sample_cache','playback_pcm','pcm']]]
    if args.owner:
        inputs[0]='tests/native_exec_amigus_wavetable_cache_test.c'
        inputs+=['src/core/amigus_reservation.c','src/core/amigus_wavetable_cache.c','src/native/amigus_reservation.c']
        lock=json.loads((ROOT/'amigus-sdk.lock.json').read_text())
        for name,expected in lock['files'].items():
            if digest(ROOT/'vendor/amigus-sdk'/name)!=expected:raise RuntimeError('SDK changed: '+name)
    flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core','-Ivendor/amigus-sdk',*compiler_safety_flags(args.cc)]
    out=ROOT/'build/dev'/('PTExecAmiGusWavetableCacheTest' if args.owner else 'PTExecAmiGusSampleRamTest');out.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run([args.cc,*flags,*inputs,'-o',str(out)],cwd=ROOT,check=True)
    dependencies=set()
    for source in inputs:
        raw=subprocess.check_output([args.cc,*flags,'-MM',source],cwd=ROOT,text=True).replace('\\\n',' ')
        dependencies.update(str((ROOT/p).resolve().relative_to(ROOT)) for p in raw.split(':',1)[1].split())
    report=dict(scope='compile/link; injected bus, no card access',flags=flags,inputs={p:digest(ROOT/p) for p in sorted(dependencies)},compiler_sha256=digest(Path(args.cc)),runtime_inputs=runtime_inputs(args.cc),binary_sha256=digest(out),binary_bytes=out.stat().st_size)
    out.with_name('amigus-wavetable-cache-build.json' if args.owner else 'amigus-sample-ram-build.json').write_text(json.dumps(report,indent=2)+'\n');print(out)
if __name__=='__main__':main()
