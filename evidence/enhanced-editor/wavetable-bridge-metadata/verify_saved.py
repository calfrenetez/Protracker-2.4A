"""Verify saved bytes only. Never invoke tools, products or targets."""
from pathlib import Path
import hashlib,json
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
pins=json.loads((BASE/'package-seal.json').read_text())
for rel,pin in pins['files'].items():
    data=(BASE/rel).read_bytes()
    assert len(data)==pin['bytes'] and hashlib.sha256(data).hexdigest()==pin['sha256'],rel
for rel,pin in pins['local_dependencies'].items():
    data=(ROOT/rel).read_bytes()
    assert len(data)==pin['bytes'] and hashlib.sha256(data).hexdigest()==pin['sha256'],rel
h=json.loads((BASE/'host/execution.json').read_text())
n=json.loads((BASE/'compiler/manifest.json').read_text())
assert h['status']=='PASS_HOST_ONLY' and n['status']=='PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED'
assert h['source_before']==h['source_after']==n['source_before']==n['source_after']
assert all(c['returncode']==0 for c in h['calls']+n['calls'])
assert len(h['calls'])==2 and len(n['calls'])==31 and len(n['units'])==22
assert 'SAMPLER METADATA PASS:8/16/24' in (BASE/'host/01.stdout').read_text()
print('Saved package and 51 project dependency records exact; host/compiler PASS; target NOT_RUN')
