#!/usr/bin/env python3
"""Build the bounded renderer snapshot fixture with production Fast allocation."""
import argparse,ast,json,subprocess
from pathlib import Path
from build_diagnostic import digest,runtime_inputs,compiler_safety_flags
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cc',required=True);args=parser.parse_args()
    tree=ast.parse((ROOT/'tests/test_render_sequence.py').read_text())
    sources=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='SOURCES' for t in n.targets))
    sources=['tests/native_exec_render_sequence_test.c',*sources[1:]]
    flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core',*compiler_safety_flags(args.cc)]
    out=ROOT/'build/dev';out.mkdir(parents=True,exist_ok=True);binary=out/'PTExecRenderSequenceTest'
    subprocess.run([args.cc,*flags,*sources,'-o',str(binary)],cwd=ROOT,check=True)
    deps=set()
    for source in sources:
        raw=subprocess.check_output([args.cc,*flags,'-MM',source],cwd=ROOT,text=True).replace('\\\n',' ')
        deps.update(str((ROOT/p).resolve().relative_to(ROOT)) for p in raw.split(':',1)[1].split())
    report=dict(flags=flags,inputs={p:digest(ROOT/p) for p in sorted(deps)},compiler_sha256=digest(Path(args.cc)),runtime_inputs=runtime_inputs(args.cc),binary_sha256=digest(binary),binary_bytes=binary.stat().st_size)
    (out/'render-sequence-build.json').write_text(json.dumps(report,indent=2)+'\n');print(binary)
if __name__=='__main__':main()
