from pathlib import Path
import hashlib,io,json,shutil,subprocess,tarfile,tempfile
ROOT=Path('/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A')
OUT=Path(tempfile.mkdtemp(prefix='protracker-paula-song-split-',dir='/private/tmp'));folder=OUT/'source';folder.mkdir()
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
raw=subprocess.check_output(['git','archive',head,'src','tests'],cwd=ROOT)
with tarfile.open(fileobj=io.BytesIO(raw)) as ar:ar.extractall(folder,filter='data')
own=['tests/paula_song_test.c','tests/test_paula_song_selected.py']
for name in own:shutil.copy2(ROOT/name,folder/name)
original=json.loads(Path('/private/tmp/protracker-paula-incremental-4glly2eg/manifest.json').read_text())
inputs=original['targets']['PTExecPaulaSongTest']['source_inputs']
assert sum(p.startswith('tests/') for p in inputs)==1 and len(inputs)==len(set(inputs))
cc='/Users/james1/Documents/Codex/2026-08-20/work-from-the-design-spec-v1/repo/.cache/amiga/bin/m68k-amigaos-gcc'
flags=original['flags']
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
assert digest(Path(cc))==original['compiler_sha256']
m={'base_commit':head,'source_export':str(folder),'scoped_overlay':{p:digest(folder/p) for p in own},'compiler_sha256':digest(Path(cc)),'compiler_version':original['compiler_version'],'runtime_inputs':original['runtime_inputs'],'preserved_failed_monolithic_sha256':original['targets']['PTExecPaulaSongTest']['binary_sha256'],'scope':'native-only per-bit full song fixture partition; unchanged production and per-bit runtime assertions; cross-build only, no target operations','targets':{}}
commands=[]
with (OUT/'native-build.log').open('w') as log:
 for bits in (8,16,24):
  name='PTExecPaulaSong'+str(bits)+'Test';binary=OUT/name;selected=[*flags,'-DPT_TEST_SONG_EXEC_BITS='+str(bits)]
  argv=[cc,*selected,*inputs,'-o',str(binary)]
  subprocess.run(argv,cwd=folder,stdout=log,stderr=log,check=True)
  deps=set()
  for source in inputs:
   raw=subprocess.check_output([cc,*selected,'-MM',source],cwd=folder,text=True).replace('\\\n',' ')
   deps.update(str((folder/p).resolve().relative_to(folder)) for p in raw.split(':',1)[1].split())
  m['targets'][name]={'selected_bits':bits,'flags':selected,'source_inputs':inputs,'dependencies':{p:digest(folder/p) for p in sorted(deps)},'binary_sha256':digest(binary),'binary_bytes':binary.stat().st_size}
  commands.append({'directory':str(folder),'file':inputs[0],'arguments':argv,'output':str(binary),'target':name,'binary_sha256':digest(binary)})
(OUT/'manifest.json').write_text(json.dumps(m,indent=2)+'\n');(OUT/'compile-commands.json').write_text(json.dumps(commands,indent=2)+'\n')
print(OUT)
print(json.dumps({name:{p:value[p] for p in ['selected_bits','binary_sha256','binary_bytes']} for name,value in m['targets'].items()},indent=2))
