#!/usr/bin/env python3
"""Build the separate two-channel EFx reference diagnostic; no guest operations."""
import argparse,json,subprocess
from pathlib import Path
from build_diagnostic import ROOT,digest,runtime_inputs,compiler_safety_flags
from prepare_invert_trace import prepare_shared_invert_trace
from prepare_invert_writes import prepare_invert_writes

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cc',required=True);parser.add_argument("--writes",action="store_true");args=parser.parse_args();record_bytes=208 if args.writes else 188
    out=ROOT/'build/dev';out.mkdir(parents=True,exist_ok=True)
    raw='vendor/pt23f/replayer/PT2.3F_replay_cia.s';wrapper='src/native/replay_abi.s'
    asm=out/('replay_invert_writes.s' if args.writes else 'replay_invert_shared.s');obj=asm.with_suffix('.o')
    prepare=prepare_invert_writes if args.writes else prepare_shared_invert_trace
    asm.write_bytes(prepare((ROOT/raw).read_bytes(),(ROOT/wrapper).read_bytes()))
    assembler=ROOT/'local/vasm/vasmm68k_mot'
    subprocess.run([str(assembler),'-devpac','-m68000','-no-fpu','-Fhunk','-o',str(obj),str(asm)],check=True)
    sources=['tests/native_flow_trace.c','src/native/paula.c',*[f'src/core/{n}.c' for n in ['document','pp20','project','mod_project','mod_inspect','channels','pcm','sample_cache','playback_pcm','pcm_filtered']]]
    flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core','-Ibuild/dev',f'-DRECORD_BYTES={record_bytes}U',*compiler_safety_flags(args.cc)]
    binary=out/('PTInvertWriteTraceTest' if args.writes else 'PTSharedInvertTraceTest')
    subprocess.run([args.cc,*flags,*sources,str(obj),'-o',str(binary)],cwd=ROOT,check=True)
    inputs={raw,wrapper,'tools/build_shared_invert_trace.py',*[f'tools/{n}.py' for n in ['prepare_replay','prepare_flow_trace','prepare_pitch_trace','prepare_sample_trace','prepare_invert_trace','prepare_invert_writes','build_diagnostic']]}
    for source in sources:
        deps=subprocess.check_output([args.cc,*flags,'-MM',source],cwd=ROOT,text=True).replace('\\\n',' ').split(':',1)[1].split()
        inputs.update(deps)
    report={'record_bytes':record_bytes,'sources':sources,'flags':flags,'inputs':{n:digest(ROOT/n) for n in sorted(inputs)},'compiler_sha256':digest(Path(args.cc)),'runtime_inputs':runtime_inputs(args.cc),'assembler_sha256':digest(assembler),'generated_source_sha256':digest(asm),'generated_object_sha256':digest(obj),'binary_sha256':digest(binary),'binary_bytes':binary.stat().st_size}
    (out/(binary.name+'-build.json')).write_text(json.dumps(report,indent=2)+'\n');print(report['binary_sha256'])
if __name__=='__main__':main()
