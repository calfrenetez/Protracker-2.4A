#!/usr/bin/env python3
"""Build ownership fixtures against committed editor sources, preserving local display work."""
import argparse,ast,hashlib,io,json,subprocess,tarfile,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
TESTS=['editor_invert_reference_test.c','native_exec_editor_invert_reference_test.c','invert_reference_oracle.h']
def committed(name):return subprocess.check_output(['git','show','HEAD:'+name],cwd=ROOT)
def constant(name,key):
    tree=ast.parse(committed(name))
    for node in tree.body:
        if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id==key for t in node.targets):return ast.literal_eval(node.value)
    raise AssertionError(key)
def prepare(folder):
    folder=Path(folder);folder.mkdir(parents=True,exist_ok=True)
    raw=subprocess.check_output(['git','archive','HEAD','src'],cwd=ROOT)
    with tarfile.open(fileobj=io.BytesIO(raw)) as archive:archive.extractall(folder,filter='data')
    (folder/'tests').mkdir(exist_ok=True)
    for name in TESTS:(folder/'tests'/name).write_bytes((ROOT/'tests'/name).read_bytes())
    (folder/'tests/native_exec_memory.h').write_bytes(committed('tests/native_exec_memory.h'))
    sources=list(dict.fromkeys(constant('tests/test_editor_studio.py','EXTRA')+constant('tests/test_editor.py','SOURCES')[1:]))
    return sources

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cc',required=True);args=parser.parse_args()
    from build_diagnostic import digest,runtime_inputs,compiler_safety_flags
    out=ROOT/'build/dev';out.mkdir(exist_ok=True,parents=True)
    folder=Path(tempfile.mkdtemp(prefix='editor-invert-reference-',dir=out));sources=prepare(folder)
    inputs=['tests/native_exec_editor_invert_reference_test.c',*sources]
    flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core',*compiler_safety_flags(args.cc)]
    binary=out/'PTExecEditorInvertReferenceTest'
    subprocess.run([args.cc,*flags,*inputs,'-o',str(binary)],cwd=folder,check=True)
    dependencies=set()
    for source in inputs:
        raw=subprocess.check_output([args.cc,*flags,'-MM',source],cwd=folder,text=True).replace('\\\n',' ')
        dependencies.update(str((folder/p).resolve().relative_to(folder)) for p in raw.split(':',1)[1].split())
    hashes={p:digest(folder/p) for p in sorted(dependencies)}
    for p,h in hashes.items():
        if p.startswith('src/'):assert hashlib.sha256(committed(p)).hexdigest()==h,p
    report=dict(source_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),flags=flags,sources=inputs,inputs=hashes,compiler_sha256=digest(Path(args.cc)),runtime_inputs=runtime_inputs(args.cc),binary_sha256=digest(binary),binary_bytes=binary.stat().st_size)
    (out/'PTExecEditorInvertReferenceTest-build.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['source_commit','binary_sha256','binary_bytes']}))
if __name__=='__main__':main()
