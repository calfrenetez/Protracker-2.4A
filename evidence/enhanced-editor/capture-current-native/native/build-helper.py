#!/usr/bin/env python3
"""Build exactly two committed capture Exec fixtures; no target or repository writes."""
import argparse,ast,hashlib,json,shlex,subprocess
from pathlib import Path
COMPILER=Path('/Users/james1/Documents/Codex/2026-08-20/work-from-the-design-spec-v1/repo/.cache/amiga/bin/m68k-amigaos-gcc')
PINNED_SHA='e3bd6818f92fe458e19d46658d2296ba85a7d8b0811571a08bea7030310dc54f'
FLAGS=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core','-Ibuild/dev','-fbbb=-']
RUNTIME=['ncrt0.o','libnix20.a','libnixmain.a','libnix.a','libstubs.a','libamiga.a','libgcc.a']
def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def run(argv,source):return subprocess.check_output(argv,cwd=source,text=True,stderr=subprocess.STDOUT)
def declarations(source):
    recipe=source/'tools/build_core_tests.py';tree=ast.parse(recipe.read_text());main=next(node for node in tree.body if isinstance(node,ast.FunctionDef) and node.name=='main')
    initial=next(node for node in main.body if isinstance(node,ast.Assign) and any(isinstance(target,ast.Name) and target.id=='inputs' for target in node.targets))
    scope={'inputs':ast.literal_eval(initial.value)}
    needed={'PTCaptureTest','PTExecCaptureTest','PTCaptureSessionTest','PTAmiGusCaptureTest','PTEditorCaptureTest','PTExecEditorCaptureTest'}
    for node in main.body:
        if not isinstance(node,ast.Assign) or len(node.targets)!=1:continue
        target=node.targets[0]
        if not isinstance(target,ast.Subscript) or not isinstance(target.value,ast.Name) or target.value.id!='inputs':continue
        if not isinstance(target.slice,ast.Constant) or target.slice.value not in needed:continue
        exec(compile(ast.Module(body=[node],type_ignores=[]),str(recipe),'exec'),{'dict':dict},scope)
    return {name:scope['inputs'][name] for name in ['PTExecCaptureTest','PTExecEditorCaptureTest']}
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--source',type=Path,required=True);parser.add_argument('--out',type=Path,required=True);args=parser.parse_args()
    source=args.source.resolve();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    provenance=source.parent/'source-manifest.json';original=json.loads(provenance.read_text())
    assert original['base_commit']=='0adacb2cef6e657931d78e4c8dc82d0ad8279601' and not original['overlay_paths'] and not original['overlays']
    for entry in original['source_inputs']:assert digest(source/entry['path'])==entry['sha256'],entry['path']
    assert digest(COMPILER)==PINNED_SHA
    reference=json.loads(Path('/private/tmp/protracker-efx-integration-final-u7z20nyz/native/manifest.json').read_text())
    manifest={'source_export':str(source),'base_commit':original['base_commit'],'base_tree':original['base_tree'],'compiler':str(COMPILER),'compiler_sha256':digest(COMPILER),'compiler_version':run([str(COMPILER),'--version'],source).splitlines()[0],'runtime_inputs':{},'runtime_paths':{},'targets':{},'source_manifest':str(provenance),'source_manifest_sha256':digest(provenance),'overlay_paths':[],'scoped_overlay':{},'scope':'Pinned committed zero-overlay host cross-build. Capture input is injected; no MMIO, target, locks, staging, hardware or timing acceptance.'}
    for name in RUNTIME:
        path=Path(run([str(COMPILER),'-m68000','-msoft-float','-mcrt=nix20','-print-file-name='+name],source).strip()).resolve();assert path.is_file();sha=digest(path);assert sha==reference['runtime_inputs'][name],name
        manifest['runtime_inputs'][name]=sha;manifest['runtime_paths'][name]={'path':str(path),'sha256':sha,'bytes':path.stat().st_size}
    files={entry['path']:entry['sha256'] for entry in original['source_inputs']};manifest['export_file_hashes']=files;manifest['export_tree_sha256']=hashlib.sha256(json.dumps(files,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    commands=[]
    for name,inputs in declarations(source).items():
        assert len(inputs)==len(set(inputs));deps={};dep_argv=[str(COMPILER),*FLAGS,'-MM',*inputs];raw=run(dep_argv,source);(out/(name+'.d')).write_text(raw)
        for line in raw.replace(chr(92)+chr(10),' ').splitlines():
            if ':' not in line:continue
            for item in shlex.split(line.split(':',1)[1]):
                path=(source/item).resolve();relative=path.relative_to(source).as_posix();assert relative in files;deps[relative]=digest(path);assert deps[relative]==files[relative]
        binary=out/name;argv=[str(COMPILER),*FLAGS,*inputs,'-o',str(binary)];log=run(argv,source);(out/(name+'.build.log')).write_text(log)
        assert binary.read_bytes()[:4]==bytes.fromhex('000003f3')
        commands.append({'name':name,'directory':str(source),'argv':argv,'output':str(binary),'ordered_translation_units':inputs,'dependency_argv':dep_argv})
        manifest['targets'][name]={'flags':FLAGS,'source_inputs':inputs,'dependencies':dict(sorted(deps.items())),'binary_sha256':digest(binary),'binary_bytes':binary.stat().st_size,'hunk_header_verified':True}
        print(name,binary.stat().st_size,digest(binary),flush=True)
    for entry in original['source_inputs']:assert digest(source/entry['path'])==entry['sha256'],entry['path']
    assert digest(COMPILER)==PINNED_SHA
    for item in manifest['runtime_paths'].values():assert digest(item['path'])==item['sha256']
    command_file=out/'compile-commands.json';command_file.write_text(json.dumps({'commands':commands},indent=2)+'\n');manifest['compile_commands']=str(command_file);manifest['compile_commands_sha256']=digest(command_file);manifest['build_helper']=str(Path(__file__).resolve());manifest['build_helper_sha256']=digest(Path(__file__));manifest['frozen_inputs_verified_before_after']=True
    manifest_file=out/'manifest.json';manifest_file.write_text(json.dumps(manifest,indent=2)+'\n');print('MANIFEST',manifest_file,digest(manifest_file),flush=True)
if __name__=='__main__':main()
