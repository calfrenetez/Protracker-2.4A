"""Verify retained bytes/results only; never compile, run or contact a target."""
from pathlib import Path
import hashlib,json
BASE=Path(__file__).resolve().parent
manifest=json.loads((BASE/'saved-bytes.json').read_text())
for name,pin in manifest['files'].items():
    data=(BASE/name).read_bytes()
    assert len(data)==pin['bytes'] and hashlib.sha256(data).hexdigest()==pin['sha256'],name
h=json.loads((BASE/'host-execution.json').read_text())
n=json.loads((BASE/'native-manifest.json').read_text())
assert h['status']=='PASS_HOST_ONLY' and h['source_stable'] and h['source_before']==h['source_after']
assert len(h['calls'])==4 and all(c['returncode']==0 for c in h['calls'])
assert n['status']=='PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED' and n['source_stable']
assert n['source_before']==n['source_after']==h['source_before']
assert len(n['calls'])==85 and len(n['dependencies'])==219 and all(c['returncode']==0 for c in n['calls'])
assert n['product']['bytes']==368616 and n['product']['sha256']=='b72a0bcd3bc58dfca2573cdeec1a915d79c958e16e4d063311ba5838171e4243'
for label in ('establishment','legacy-link'):
    for stage in ('compile','run'):assert not (BASE/(label+'-'+stage+'.stderr')).read_bytes()
assert 'EDITOR ESTABLISH PASS:27' in (BASE/'establishment-run.stdout').read_text()
assert '24 allocator control-alias cases;15 legacy transport regressions' in (BASE/'establishment-run.stdout').read_text()
assert 'EDITOR MIXED PASS:15' in (BASE/'legacy-link-run.stdout').read_text()
failed=json.loads((BASE/'first-link-failure.json').read_text())
assert failed['status']=='FIRST_FAILURE_RETAINED' and len(failed['calls'])==1 and failed['calls'][0]['returncode']!=0
assert json.loads((BASE/'runner-import-refusal.json').read_text())['candidate_calls']==0
print('PASS saved editor cancellation host/compiler evidence; native/physical NOT_RUN; no producer/target replay')
