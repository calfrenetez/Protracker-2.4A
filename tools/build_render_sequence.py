#!/usr/bin/env python3
"""Build bounded renderer snapshot or exact cache-restore fixtures."""
import argparse,ast,json,subprocess
from pathlib import Path
from build_diagnostic import digest,runtime_inputs,compiler_safety_flags
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cc',required=True);parser.add_argument('--fixture',choices=['render-sequence','amigus-restore'],default='render-sequence');args=parser.parse_args()
    test='test_render_sequence.py' if args.fixture=='render-sequence' else 'test_amigus_restore.py'
    tree=ast.parse((ROOT/'tests'/test).read_text())
    sources=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='SOURCES' for t in n.targets))
    if args.fixture=='render-sequence':sources=['tests/native_exec_render_sequence_test.c',*sources[1:]]
    flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core',*compiler_safety_flags(args.cc)]
    out=ROOT/'build/dev';out.mkdir(parents=True,exist_ok=True);binary=out/('PTExecRenderSequenceTest' if args.fixture=='render-sequence' else 'PTAmiGusRestoreTest')
    subprocess.run([args.cc,*flags,*sources,'-o',str(binary)],cwd=ROOT,check=True)
    syntax=['src/editor/wavetable_dispatch.c'] if args.fixture=='amigus-restore' else []
    for source in syntax:subprocess.run([args.cc,*flags,'-fsyntax-only',source],cwd=ROOT,check=True)
    deps=set()
    for source in sources+syntax:
        raw=subprocess.check_output([args.cc,*flags,'-MM',source],cwd=ROOT,text=True).replace('\\\n',' ')
        deps.update(str((ROOT/p).resolve().relative_to(ROOT)) for p in raw.split(':',1)[1].split())
    report=dict(syntax_only=syntax,flags=flags,inputs={p:digest(ROOT/p) for p in sorted(deps)},compiler_sha256=digest(Path(args.cc)),runtime_inputs=runtime_inputs(args.cc),binary_sha256=digest(binary),binary_bytes=binary.stat().st_size)
    (out/(args.fixture+'-build.json')).write_text(json.dumps(report,indent=2)+'\n');print(binary)
if __name__=='__main__':main()
