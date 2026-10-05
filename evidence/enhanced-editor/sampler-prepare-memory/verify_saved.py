"""Saved bytes only; no compiler, executable, transport or target access."""
from pathlib import Path
import hashlib,json
B=Path(__file__).resolve().parent
R=B.parents[2]
s=json.loads((B/'package-seal.json').read_text())
for rel,pin in s['files'].items():
    x=(B/rel).read_bytes()
    assert len(x)==pin['bytes'] and hashlib.sha256(x).hexdigest()==pin['sha256'],rel
for rel,pin in s['local_dependencies'].items():
    x=(R/rel).read_bytes()
    assert len(x)==pin['bytes'] and hashlib.sha256(x).hexdigest()==pin['sha256'],rel
h=json.loads((B/'host/execution.json').read_text())
n=json.loads((B/'compiler/manifest.json').read_text())
f=json.loads((B/'first-compile-failure/execution.json').read_text())
assert h['status']=='PASS_HOST_ONLY' and n['status']=='PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED'
assert h['source_before']==h['source_after']==n['source_before']==n['source_after']
assert len(h['calls'])==2 and len(n['calls'])==47 and len(n['units'])==38
assert all(c['returncode']==0 for c in h['calls']+n['calls'])
assert f['status']=='FIRST_FAILURE_RETAINED' and len(f['calls'])==1
assert f['calls'][0]['returncode']!=0 and not f['calls'][0].get('product')
assert 'PREPARE MEMORY PASS:21 known aliases' in (B/'host/01.stdout').read_text()
print('Saved bytes/87 project dependencies exact; changed host/compiler PASS; first failure retained; targets NOT_RUN')
