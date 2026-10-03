#!/usr/bin/env python3
"""Build the three EFx Exec fixtures from an already-frozen scoped source export.
No export, repository mutation, staging or target execution occurs here.
"""
import argparse, hashlib, importlib, json, shlex, subprocess, sys
from pathlib import Path

COMPILER=Path('/Users/james1/Documents/Codex/2026-08-20/work-from-the-design-spec-v1/repo/.cache/amiga/bin/m68k-amigaos-gcc')
FLAGS=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core','-fbbb=-']
RUNTIME=['ncrt0.o','libnix20.a','libnixmain.a','libnix.a','libstubs.a','libamiga.a','libgcc.a']
def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def run(argv,source):return subprocess.check_output(argv,cwd=source,text=True,stderr=subprocess.STDOUT)
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();source=args.source.resolve();out=args.out.resolve()
    assert source.is_dir() and COMPILER.is_file()
    out.mkdir(parents=True,exist_ok=True);sys.dont_write_bytecode=True;sys.path.insert(0,str(source/'tests'))
    core=importlib.import_module('test_render_invert_session').SOURCES
    sampler=importlib.import_module('test_sampler_invert_song').SOURCES
    editor=importlib.import_module('test_editor').SOURCES
    extra=importlib.import_module('test_editor_studio').EXTRA
    targets={
        'PTExecInvertSessionTest':['tests/native_exec_invert_session_test.c',*[f'src/core/{name}.c' for name in core]],
        'PTExecSamplerInvertSongTest':['tests/native_exec_sampler_invert_song_test.c',*sampler],
        'PTExecEditorInvertStudioTest':['tests/native_exec_editor_invert_studio_test.c',*extra,*editor[1:]],
    }
    manifest={'source_export':str(source),'compiler':str(COMPILER),'compiler_sha256':digest(COMPILER),
        'compiler_version':run([str(COMPILER),'--version'],source).splitlines()[0],
        'runtime_inputs':{},'targets':{},
        'scope':'Pinned host cross-build only. Existing Exec wrappers include fixture C as dependencies, never duplicate translation units. No target/card/device/physical operation or timing acceptance.'}
    for name in RUNTIME:
        path=Path(run([str(COMPILER),'-m68000','-msoft-float','-mcrt=nix20','-print-file-name='+name],source).strip())
        assert path.is_file();manifest['runtime_inputs'][name]=digest(path)
    provenance=source.parent/'source-manifest.json'
    if provenance.is_file():
        manifest['source_manifest']=str(provenance);manifest['source_manifest_sha256']=digest(provenance)
        original=json.loads(provenance.read_text())
        for key in ('base_commit','baseline_commit','base_tree','baseline_tree','scoped_overlay','overlay_paths','source_tree_sha256'):
            if key in original:manifest[key]=original[key]
        manifest['scoped_overlay']={row['path']:row['sha256'] for row in original.get('source_inputs',[]) if row.get('overlay')}
    files={p.relative_to(source).as_posix():digest(p) for p in sorted(source.rglob('*'))
        if p.is_file() and '__pycache__' not in p.parts}
    manifest['export_file_hashes']=files
    manifest['export_tree_sha256']=hashlib.sha256(json.dumps(files,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    commands=[]
    for name,inputs in targets.items():
        assert len(inputs)==len(set(inputs)),name
        deps={}
        dep_text=run([str(COMPILER),*FLAGS,'-MM',*inputs],source).replace('\\\n',' ')
        for line in dep_text.splitlines():
            if ':' not in line:continue
            for item in shlex.split(line.split(':',1)[1]):
                path=(source/item).resolve();relative=path.relative_to(source).as_posix()
                deps[relative]=digest(path)
        binary=out/name;argv=[str(COMPILER),*FLAGS,*inputs,'-o',str(binary)]
        command={'name':name,'directory':str(source),'argv':argv,'output':str(binary),'ordered_translation_units':inputs}
        commands.append(command);log=run(argv,source);(out/(name+'.build.log')).write_text(log)
        manifest['targets'][name]={'flags':FLAGS,'source_inputs':inputs,'dependencies':dict(sorted(deps.items())),
            'binary_sha256':digest(binary),'binary_bytes':binary.stat().st_size}
        print(name,binary.stat().st_size,digest(binary),flush=True)
    command_file=out/'compile-commands.json';command_file.write_text(json.dumps({'commands':commands},indent=2)+'\n')
    manifest['compile_commands']=str(command_file);manifest['compile_commands_sha256']=digest(command_file)
    manifest['build_helper']=str(Path(__file__).resolve());manifest['build_helper_sha256']=digest(Path(__file__))
    manifest_file=out/'manifest.json';manifest_file.write_text(json.dumps(manifest,indent=2)+'\n')
    print('MANIFEST',manifest_file,digest(manifest_file),flush=True)
if __name__=='__main__':main()
