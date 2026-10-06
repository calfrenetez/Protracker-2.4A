"""Audit saved bytes/results only; no compilation, execution or target operation."""
from pathlib import Path
import hashlib,json
BASE=Path(__file__).resolve().parent
manifest=json.loads((BASE/'saved-bytes.json').read_text())
for name,pin in manifest['files'].items():
    data=(BASE/name).read_bytes()
    assert len(data)==pin['bytes'] and hashlib.sha256(data).hexdigest()==pin['sha256'],name
h=json.loads((BASE/'host-execution.json').read_text())
n=json.loads((BASE/'native-manifest.json').read_text())
s=json.loads((BASE/'source-controls.json').read_text())
assert h['status']=='PASS_HOST_ONLY' and h['source_stable'] and h['source_before']==h['source_after']
assert len(h['source_before'])==905 and h['source_before']==s['source_files']
assert len(s['own_paths'])==9 and len(s['protected'])==16
assert len(h['calls'])==6 and all(c['returncode']==0 for c in h['calls'])
assert n['status']=='PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED' and n['source_stable']
assert n['source_before']==n['source_after']==h['source_before']
assert len(n['calls'])==89 and len(n['dependencies'])==228 and all(c['returncode']==0 for c in n['calls'])
assert n['product']['bytes']==378988 and n['product']['sha256']=='f37fef467d74e4684a6663350f25fd8ba7c30f8e43056e48896351af6faf2469'
for label in ('checked','establishment','original-established'):
    for stage in ('compile','run'):assert not (BASE/(label+'-'+stage+'.stderr')).read_bytes()
checked=(BASE/'checked-run.stdout').read_text()
assert 'EDITOR CHECKED PASS:39' in checked and '18 whole-control allocator-alias' in checked and '15 legacy transport regressions' in checked
established=(BASE/'original-established-run.stdout').read_text()
assert 'ESTABLISHED OWNER PASS:66' in established and 'full255 host scenarios' in established
prior=(BASE/'establishment-run.stdout').read_text()
assert 'EDITOR ESTABLISH PASS:27' in prior and '24 allocator control-alias cases' in prior
failed=json.loads((BASE/'first-compile-failure.json').read_text())
assert failed['status']=='FIRST_FAILURE_RETAINED' and failed['source_stable']
assert len(failed['calls'])==1 and failed['calls'][0]['stage']=='compile' and failed['calls'][0]['returncode']!=0
first=json.loads((BASE/'first-source-controls.json').read_text())
assert set(first['source_files'])==set(s['source_files'])
assert [k for k,v in first['source_files'].items() if v!=s['source_files'][k]]==['tests/editor_mixed_checked_test.c']
assert 'incompatible' in (BASE/'first-compile.stderr').read_text()
logs=json.loads((BASE/'native-log-audit.json').read_text())['files']
assert len([k for k in logs if k.endswith('.stderr')])==89
assert all(v['bytes']==0 for k,v in logs.items() if k.endswith('.stderr'))
print('PASS saved editor checked lifetime host/compiler evidence; native/physical NOT_RUN; no producer/target replay')
