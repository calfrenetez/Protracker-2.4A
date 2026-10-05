from pathlib import Path
import hashlib,json,importlib.util
BASE=Path(__file__).resolve().parent
manifest=json.loads((BASE/'saved-bytes-v1.json').read_text())
for name,pin in manifest['files'].items():
 b=(BASE/name).read_bytes();assert len(b)==pin['bytes'] and hashlib.sha256(b).hexdigest()==pin['sha256'],name
h=json.loads((BASE/'host-execution.json').read_text());n=json.loads((BASE/'native-manifest.json').read_text())
assert h['status']=='PASS_HOST_ONLY' and h['source_stable'] and h['source_before']==h['source_after']
assert len(h['calls'])==4 and all(c['returncode']==0 for c in h['calls'])
assert n['status']=='PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED' and n['source_stable'] and n['source_before']==n['source_after']==h['source_before']
assert len(n['calls'])==55 and all(c['returncode']==0 for c in n['calls'])
assert n['product']['bytes']==292912 and n['product']['sha256']=='2c88205c370a3ab8d68a0f5beecc851d0d9e44e9e90a95d590a74185d03348e8'
assert not (BASE/'host-progress.stderr').read_bytes() and not (BASE/'host-sampler.stderr').read_bytes()
tool=BASE.parents[2]/'tools/native_fixture_observation.py'
spec=importlib.util.spec_from_file_location('observation',tool);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
r=module.progress_snapshot((BASE/'host-progress.stdout').read_bytes(),True)
assert r['completedCases']==342 and r['progressRecords']==684 and r['ended']
assert json.loads((BASE/'observation-tests-v1.json').read_text())['returncode']==0
print('PASS saved observable fixture host/compiler evidence; no target/candidate replay')
