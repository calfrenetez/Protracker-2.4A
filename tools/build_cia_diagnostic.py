#!/usr/bin/env python3
"""Build the staged timer-only CIA diagnostic, never the dirty classic frontend."""
import argparse,hashlib,io,json,subprocess,tarfile,tempfile
from pathlib import Path
from build_diagnostic import ROOT,digest,runtime_inputs,compiler_safety_flags

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cc',required=True);args=p.parse_args()
    tree=subprocess.check_output(['git','write-tree'],cwd=ROOT,text=True).strip()
    out=ROOT/'build/dev';out.mkdir(parents=True,exist_ok=True)
    folder=Path(tempfile.mkdtemp(prefix='cia-diagnostic-',dir=out))
    raw=subprocess.check_output(['git','archive',tree,'src','tests'],cwd=ROOT)
    with tarfile.open(fileobj=io.BytesIO(raw)) as ar:ar.extractall(folder,filter='data')
    assembler=ROOT/'local/vasm/vasmm68k_mot';objectfile=folder/'cia_irq.o'
    assembler_flags=['-devpac','-m68000','-no-fpu','-Fhunk']
    subprocess.run([str(assembler),*assembler_flags,'-o',str(objectfile),'tests/native_cia_irq.s'],cwd=folder,check=True)
    flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os',
           '-Wall','-Wextra','-Werror','-Isrc/core',*compiler_safety_flags(args.cc)]
    sources=['tests/native_exec_cia_timing_test.c','src/core/elapsed_clock.c']
    binary=out/'PTExecCiaTimingTest'
    subprocess.run([args.cc,*flags,*sources,str(objectfile),'-o',str(binary)],cwd=folder,check=True)
    dependencies={'tests/native_cia_irq.s'}
    for source in sources:
        raw=subprocess.check_output([args.cc,*flags,'-MM',source],cwd=folder,text=True).replace('\\\n',' ')
        dependencies.update(str((folder/name).resolve().relative_to(folder)) for name in raw.split(':',1)[1].split())
    # System headers also define the interrupt ABI/resource and timer vectors.
    sdk_inputs={}
    for source in sources:
        raw=subprocess.check_output([args.cc,*flags,'-M',source],cwd=folder,text=True).replace('\\\n',' ')
        for name in raw.split(':',1)[1].split():
            path=Path(name)
            if path.is_absolute():sdk_inputs[str(path)]=digest(path)
    report=dict(source_tree=tree,source_export=str(folder),flags=flags,
        inputs={name:digest(folder/name) for name in sorted(dependencies)},sdk_inputs=sdk_inputs,
        builder_sha256=hashlib.sha256(subprocess.check_output(['git','show',tree+':tools/build_cia_diagnostic.py'],cwd=ROOT)).hexdigest(),
        compiler=subprocess.check_output([args.cc,'--version'],text=True).splitlines()[0],
        compiler_sha256=digest(Path(args.cc)),runtime_inputs=runtime_inputs(args.cc),
        assembler_sha256=digest(assembler),assembler_flags=assembler_flags,
        binary_bytes=binary.stat().st_size,binary_sha256=digest(binary),
        scope='timer-only CIA acquisition/IRQ timestamp/teardown; no audio or frontend')
    (out/'cia-diagnostic-build.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
