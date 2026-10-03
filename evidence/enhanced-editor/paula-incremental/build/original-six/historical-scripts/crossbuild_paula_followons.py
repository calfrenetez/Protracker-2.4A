from pathlib import Path
import hashlib,json,subprocess
out=Path('/private/tmp/protracker-paula-incremental-4glly2eg');manifest_path=out/'manifest.json'
m=json.loads(manifest_path.read_text());folder=Path(m['source_export'])
cc='/Users/james1/Documents/Codex/2026-08-20/work-from-the-design-spec-v1/repo/.cache/amiga/bin/m68k-amigaos-gcc';flags=m['flags']
sources=m['targets']['PTExecEditorPaulaTest']['source_inputs'][1:]
targets={'PTExecPreparedPaulaTest':'tests/native_exec_prepared_paula_test.c','PTExecPaulaTransportTest':'tests/native_exec_paula_transport_test.c','PTExecPaulaWaitTest':'tests/native_exec_paula_wait_test.c'}
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
with (out/'native-followons-build.log').open('w') as log:
 for name,fixture in targets.items():
  inputs=[fixture,*sources];binary=out/name
  subprocess.run([cc,*flags,*inputs,'-o',str(binary)],cwd=folder,stdout=log,stderr=log,check=True)
  deps=set()
  for source in inputs:
   raw=subprocess.check_output([cc,*flags,'-MM',source],cwd=folder,text=True).replace('\\\n',' ')
   deps.update(raw.split(':',1)[1].split())
  m['targets'][name]={'source_inputs':inputs,'dependencies':{p:digest(folder/p) for p in sorted(deps)},'binary_sha256':digest(binary),'binary_bytes':binary.stat().st_size,'acceptance':'cross-build only; no emulator or physical execution'}
manifest_path.write_text(json.dumps(m,indent=2)+'\n')
print(json.dumps({name:{key:m['targets'][name][key] for key in ['binary_sha256','binary_bytes']} for name in targets},indent=2))
