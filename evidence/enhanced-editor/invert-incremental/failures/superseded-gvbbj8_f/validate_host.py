from pathlib import Path
import hashlib,json,os,shlex,shutil,subprocess,sys,time,unittest
artifact=Path(__file__).resolve().parent
root=artifact/'source'
sys.dont_write_bytecode=True
sys.path.insert(0,str(root/'tests'))
os.chdir(root)
manifest=json.loads((artifact/'source-manifest.json').read_text())
def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def verify():
    for item in manifest['source_inputs']:
        path=root/item['path']
        if digest(path)!=item['sha256']:raise RuntimeError('Frozen source mismatch: '+item['path'])
verify()
real_run=subprocess.run
compiler=Path(shutil.which('cc')).resolve()
compiler_info={'path':str(compiler),'sha256':digest(compiler),'version':real_run([str(compiler),'--version'],check=True,stdout=subprocess.PIPE,text=True).stdout,'resource_dir':real_run([str(compiler),'-print-resource-dir'],check=True,stdout=subprocess.PIPE,text=True).stdout.strip()}
runtime=[]
for library in ['libclang_rt.asan_osx_dynamic.dylib','libclang_rt.ubsan_osx_dynamic.dylib']:
    path=Path(real_run([str(compiler),'-print-file-name='+library],check=True,stdout=subprocess.PIPE,text=True).stdout.strip()).resolve()
    if not path.is_file():raise RuntimeError('Missing sanitizer runtime: '+str(path))
    runtime.append({'path':str(path),'sha256':digest(path),'size':path.stat().st_size})
commands=[];dependencies=set();(artifact/'host-binaries').mkdir(exist_ok=True)
def recorded_run(argv,*args,**kwargs):
    command={'argv':[str(x) for x in argv],'cwd':str(kwargs.get('cwd',Path.cwd())),'started':time.time()}
    commands.append(command)
    try:
        result=real_run(argv,*args,**kwargs);command['returncode']=result.returncode
    except subprocess.CalledProcessError as error:
        command['returncode']=error.returncode;raise
    finally:command['elapsed_s']=time.time()-command['started']
    if argv and Path(str(argv[0])).name in ['cc','clang'] and '-o' in argv:
        binary=Path(argv[argv.index('-o')+1]);number=sum('binary' in x for x in commands)+1
        archived=artifact/'host-binaries'/('suite-'+str(number))
        shutil.copyfile(binary,archived);archived.chmod(0o555)
        command['binary']={'path':str(archived),'sha256':digest(archived),'size':archived.stat().st_size}
        inputs=[str(x) for x in argv if str(x).endswith('.c')]
        dep_argv=[str(compiler),'-std=c99','-Isrc/core','-MM',*inputs]
        dep=real_run(dep_argv,cwd=root,check=True,stdout=subprocess.PIPE,text=True)
        (artifact/('suite-'+str(number)+'.d')).write_text(dep.stdout)
        command['dependency_command']=dep_argv
        for line in dep.stdout.replace('\
','').splitlines():
            if ':' not in line:continue
            for value in shlex.split(line.split(':',1)[1]):
                path=(root/value).resolve();relative=path.relative_to(root).as_posix()
                if relative not in {x['path'] for x in manifest['source_inputs']}:raise RuntimeError('Noncanonical dependency: '+relative)
                dependencies.add(relative)
    return result
subprocess.run=recorded_run
modules=['test_sampler_invert_song','test_editor_invert_studio','test_editor_studio']
start=time.monotonic();result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromNames(modules))
verify()
python_inputs=[]
for name,module in sorted(sys.modules.items()):
    value=getattr(module,'__file__',None)
    if not value:continue
    path=Path(value).resolve()
    try:relative=path.relative_to(root).as_posix()
    except ValueError:continue
    if path.is_file():
        python_inputs.append({'module':name,'path':relative,'sha256':digest(path)});dependencies.add(relative)
index={x['path']:x for x in manifest['source_inputs']}
evidence={'base_commit':manifest['base_commit'],'source_export':str(root),'passed':result.wasSuccessful(),'tests_run':result.testsRun,'failures':len(result.failures),'errors':len(result.errors),'elapsed_s':time.monotonic()-start,'compiler':compiler_info,'sanitizer_runtime_inputs':runtime,'commands':commands,'python_imports':python_inputs,'canonical_dependencies':[index[x] for x in sorted(dependencies)],'frozen_input_checks_before_after':True,'guest_access':False,'staged':False}
(artifact/'host-checks.json').write_text(json.dumps(evidence,indent=2)+'\n')
raise SystemExit(0 if result.wasSuccessful() else 1)
