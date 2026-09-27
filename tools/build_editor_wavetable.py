#!/usr/bin/env python3
"""Validate staged/committed editor ownership without unrelated working-tree display edits."""
import argparse,ast,hashlib,io,json,subprocess,tarfile,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def prepare(folder):
    tree=subprocess.check_output(['git','write-tree'],cwd=ROOT,text=True).strip()
    folder=Path(folder);folder.mkdir(parents=True,exist_ok=True)
    raw=subprocess.check_output(['git','archive',tree,'src','tests'],cwd=ROOT)
    with tarfile.open(fileobj=io.BytesIO(raw)) as ar:ar.extractall(folder,filter='data')
    def constant(name,key):
        return next(ast.literal_eval(n.value) for n in ast.parse((folder/name).read_text()).body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==key for t in n.targets))
    sources=list(dict.fromkeys(constant('tests/test_editor.py','SOURCES')[1:]+constant('tests/test_editor_studio.py','EXTRA')+constant('tests/test_wavetable_dispatch.py','DISPATCH')+['src/editor/sampler_wavetable.c']+['src/core/'+n+'.c' for n in ['amigus_reservation','amigus_wavetable_cache','amigus_sample_ram','sample_cache','playback_pcm']]+['src/editor/editor_wavetable.c','src/platform/sample_import.c','src/platform/raw_import.c','src/platform/mod_import.c','src/platform/pp20_import.c']))
    return sources,tree

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cc',required=True)
    group=parser.add_mutually_exclusive_group()
    group.add_argument('--studio-output',action='store_true',help='Build editor Studio producer/output ownership fixture')
    group.add_argument('--timing-only',action='store_true',help='Build the finite no-argument clock/memory/fake-voice diagnostic')
    args=parser.parse_args()
    from build_diagnostic import digest,runtime_inputs,compiler_safety_flags
    out=ROOT/'build/dev';out.mkdir(parents=True,exist_ok=True)
    folder=Path(tempfile.mkdtemp(prefix='editor-wavetable-',dir=out));sources,tree=prepare(folder)
    inputs=['tests/native_exec_editor_studio_test.c' if args.studio_output else 'tests/native_exec_editor_wavetable_test.c',*sources]
    font=subprocess.check_output(['git','show',tree+':vendor/pt23f/raw/ptfont.raw'],cwd=ROOT)
    (folder/'pt_font.h').write_text('static const unsigned char pt_font[580] = {'+','.join(str(b) for b in font)+'};\n')
    flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core','-I.',*compiler_safety_flags(args.cc)]
    if args.timing_only:flags+=['-DPT_NATIVE_TIMING_ONLY']
    binary=out/('PTExecEditorStudioTest' if args.studio_output else 'PTExecWavetableTimingTest' if args.timing_only else 'PTExecEditorWavetableTest')
    subprocess.run([args.cc,*flags,*inputs,'-o',str(binary)],cwd=folder,check=True)
    subprocess.run([args.cc,*flags,'-fsyntax-only','src/native/editor_main.c'],cwd=folder,check=True)
    deps=set()
    for source in inputs+['src/native/editor_main.c']:
        raw=subprocess.check_output([args.cc,*flags,'-MM',source],cwd=folder,text=True).replace('\\\n',' ')
        deps.update(str((folder/p).resolve().relative_to(folder)) for p in raw.split(':',1)[1].split())
    report=dict(font_source_sha256=hashlib.sha256(font).hexdigest(),source_tree=tree,source_export=str(folder),flags=flags,inputs={p:digest(folder/p) for p in sorted(deps)},compiler_sha256=digest(Path(args.cc)),runtime_inputs=runtime_inputs(args.cc),binary_sha256=digest(binary),binary_bytes=binary.stat().st_size)
    (out/('editor-studio-output-build.json' if args.studio_output else 'wavetable-timing-build.json' if args.timing_only else 'editor-wavetable-build.json')).write_text(json.dumps(report,indent=2)+'\n');print(binary)
if __name__=='__main__':main()
