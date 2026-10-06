"""Audit saved bytes/results only; never compile, execute or contact transport."""
from pathlib import Path
import hashlib,json
BASE=Path(__file__).resolve().parent
for name,pin in json.loads((BASE/'saved-bytes.json').read_text())['files'].items():
    b=(BASE/name).read_bytes();assert len(b)==pin['bytes'] and hashlib.sha256(b).hexdigest()==pin['sha256'],name
h=json.loads((BASE/'host-execution.json').read_text());n=json.loads((BASE/'native-manifest.json').read_text())
s=json.loads((BASE/'source-controls.json').read_text())
assert h['status']=='PASS_HOST_ONLY' and h['source_stable'] and h['source_before']==h['source_after']==s['source_files']
assert len(h['source_before'])==909 and len(s['own_paths'])==5 and len(s['protected'])==16
assert len(h['calls'])==6 and all(c['returncode']==0 for c in h['calls'])
assert n['status']=='PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED' and n['source_stable']
assert n['source_before']==n['source_after']==h['source_before']
assert len(n['calls'])==90 and len(n['dependencies'])==231 and all(c['returncode']==0 for c in n['calls'])
assert n['product']['bytes']==385144 and n['product']['sha256']=='21c5efc7fe5d066f56e81ea8cbcea8bc7e95892e8af0a55608a1410d882d9b1e'
for label in ('bridges','checked-standalone','original-established'):
    for stage in ('compile','run'):assert not (BASE/(label+'-'+stage+'.stderr')).read_bytes()
text=(BASE/'bridges-run.stdout').read_text()
assert 'EDITOR BRIDGES PASS:9' in text and '75 protected-output/context/cache/admission refusals' in text
assert 'EDITOR CHECKED PASS:39' in text and '18 whole-control allocator-alias' in text and 'EDITOR MIXED PASS:15' in text
assert 'EDITOR CHECKED PASS:39' in (BASE/'checked-standalone-run.stdout').read_text()
text=(BASE/'original-established-run.stdout').read_text();assert 'ESTABLISHED OWNER PASS:66' in text and 'full255 host scenarios' in text
logs=json.loads((BASE/'native-log-audit.json').read_text())['files']
assert len([k for k in logs if k.endswith('.stderr')])==90
assert all(v['bytes']==0 for k,v in logs.items() if k.endswith('.stderr'))
prior=json.loads((BASE/'prior-version-summary.json').read_text())
assert prior['host_status']=='PASS_HOST_ONLY' and prior['host_calls']==6
assert prior['native_status']=='PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED' and prior['native_calls']==90
print('PASS saved editor metadata bridge host/compiler evidence; native/physical NOT_RUN; no producer/target replay')
