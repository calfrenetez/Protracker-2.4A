from pathlib import Path
import ast,hashlib,io,json,os,shutil,subprocess,sys,tarfile,tempfile,unittest
ROOT=Path('/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A')
OUT=Path(tempfile.mkdtemp(prefix='protracker-paula-incremental-',dir='/private/tmp'))
EXPORT=OUT/'source';EXPORT.mkdir()
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
raw=subprocess.check_output(['git','archive',head,'src','tests'],cwd=ROOT)
with tarfile.open(fileobj=io.BytesIO(raw)) as ar:ar.extractall(EXPORT,filter='data')
OWN=['src/editor/paula_preflight.c','src/editor/paula_preflight.h','src/editor/paula_song.c','src/editor/paula_song.h','src/native/editor_paula.h','tests/paula_preflight_test.c','tests/paula_song_test.c','tests/editor_paula_test.c','tests/native_editor_paula_test.c','tests/native_paula_transport_test.c','tests/native_exec_paula_transport_test.c','tests/native_exec_paula_wait_test.c','tests/native_exec_prepared_paula_test.c']
for name in OWN:shutil.copy2(ROOT/name,EXPORT/name)
def constant(name,key):
 return next(ast.literal_eval(n.value) for n in ast.parse((EXPORT/name).read_text()).body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==key for t in n.targets))
def unique(items):return list(dict.fromkeys(items))
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
editor=unique(constant('tests/test_editor.py','SOURCES')[1:]+constant('tests/test_editor_studio.py','EXTRA')+constant('tests/test_wavetable_dispatch.py','DISPATCH')+['src/editor/sampler_wavetable.c']+['src/core/'+n+'.c' for n in ['amigus_reservation','amigus_wavetable_cache','amigus_sample_ram','sample_cache','playback_pcm']]+['src/editor/editor_wavetable.c','src/platform/sample_import.c','src/platform/raw_import.c','src/platform/mod_import.c','src/platform/pp20_import.c'])
def prepare(folder):
 folder=Path(folder);shutil.copytree(EXPORT,folder,dirs_exist_ok=True);return editor,head
sys.path[:0]=[str(EXPORT/'tests'),str(ROOT/'tools')]
import build_editor_wavetable
build_editor_wavetable.prepare=prepare
modules=['test_paula_preflight','test_paula_song','test_paula_dispatch','test_editor_paula','test_native_editor_paula','test_native_paula_transport','test_mixed_preflight','test_mixed_owner']
suite=unittest.TestSuite(unittest.defaultTestLoader.loadTestsFromName(name) for name in modules)
with (OUT/'host.log').open('w') as log:
 saved_stdout,saved_stderr=sys.stdout,sys.stderr
 sys.stdout=sys.stderr=log
 # subprocesses use inherited descriptors, so retain their assertion output too.
 oldout,olderr=os.dup(1),os.dup(2)
 os.dup2(log.fileno(),1);os.dup2(log.fileno(),2)
 try:result=unittest.TextTestRunner(stream=log,verbosity=2).run(suite)
 finally:
  os.dup2(oldout,1);os.dup2(olderr,2);os.close(oldout);os.close(olderr);sys.stdout,sys.stderr=saved_stdout,saved_stderr
print(OUT,flush=True)
if not result.wasSuccessful():
 print((OUT/'host.log').read_text());raise SystemExit(1)
pref=constant('tests/test_paula_preflight.py','SOURCES')[1:]
sampler=constant('tests/test_sampler.py','SOURCES')[1:]
song=unique(['src/editor/paula_song.c','src/core/elapsed_clock.c','src/editor/paula_dispatch.c','src/editor/paula_voices.c','src/editor/sampler_paula.c','src/core/sample_cache.c','src/core/playback_pcm.c',*sampler,*pref])
paula=constant('tests/test_editor_paula.py','PAULA')
cc=Path('/Users/james1/Documents/Codex/2026-08-20/work-from-the-design-spec-v1/repo/.cache/amiga/bin/m68k-amigaos-gcc')
flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core','-I.','-fbbb=-']
targets={'PTExecPaulaPreflightTest':['tests/native_exec_paula_preflight_test.c',*pref], 'PTExecPaulaSongTest':['tests/native_exec_paula_song_test.c',*song], 'PTExecEditorPaulaTest':['tests/native_exec_editor_paula_test.c',*unique([*paula,*editor])]}
import build_diagnostic
manifest={'base_commit':head,'source_export':str(EXPORT),'scoped_overlay':{name:digest(EXPORT/name) for name in OWN},'host_modules':modules,'host_success':True,'flags':flags,'compiler_sha256':digest(cc),'compiler_version':subprocess.check_output([str(cc),'--version'],text=True).splitlines()[0],'runtime_inputs':build_diagnostic.runtime_inputs(str(cc)),'targets':{}}
with (OUT/'native-build.log').open('w') as log:
 for name,inputs in targets.items():
  binary=OUT/name;subprocess.run([str(cc),*flags,*inputs,'-o',str(binary)],cwd=EXPORT,stdout=log,stderr=log,check=True)
  deps=set()
  for source in inputs:
   raw=subprocess.check_output([str(cc),*flags,'-MM',source],cwd=EXPORT,text=True).replace('\\\n',' ')
   deps.update(raw.split(':',1)[1].split())
  manifest['targets'][name]={'source_inputs':inputs,'dependencies':{p:digest(EXPORT/p) for p in sorted(deps)},'binary_sha256':digest(binary),'binary_bytes':binary.stat().st_size}
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps({name:{key:data[key] for key in ['binary_sha256','binary_bytes']} for name,data in manifest['targets'].items()},indent=2))
