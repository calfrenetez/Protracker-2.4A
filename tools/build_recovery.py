#!/usr/bin/env python3
"""Build the explicit recovery-file fixture; no automatic snapshots or UI changes."""
import argparse,ast,json,subprocess
from pathlib import Path
from build_diagnostic import ROOT,digest,compiler_safety_flags,runtime_inputs
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cc',required=True);args=parser.parse_args()
    tree=ast.parse((ROOT/'tests/test_recovery_file.py').read_text())
    sources=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='SOURCES' for t in n.targets))
    sources=['tests/native_exec_recovery_test.c',*sources]
    flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core',*compiler_safety_flags(args.cc)]
    out=ROOT/'build/dev/PTExecRecoveryTest';out.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run([args.cc,*flags,*sources,'-o',str(out)],cwd=ROOT,check=True)
    deps=set()
    for source in sources:
        raw=subprocess.check_output([args.cc,*flags,'-MM',source],cwd=ROOT,text=True).replace('\\\n',' ')
        deps.update(str((ROOT/p).resolve().relative_to(ROOT)) for p in raw.split(':',1)[1].split())
    report=dict(scope='compile/link only; explicit recovery file transactions',sources=sources,flags=flags,inputs={p:digest(ROOT/p) for p in sorted(deps)},compiler_sha256=digest(Path(args.cc)),runtime_inputs=runtime_inputs(args.cc),binary_bytes=out.stat().st_size,binary_sha256=digest(out))
    (out.parent/'recovery-build.json').write_text(json.dumps(report,indent=2)+'\n');print(out)
if __name__=='__main__':main()
