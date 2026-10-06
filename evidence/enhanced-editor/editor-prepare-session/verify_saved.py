"""Saved host evidence only; no compiler, producer or target execution."""
import hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent
def read(n):return json.loads((HERE/n).read_text())
def fp(p):
    assert p.is_file() and not p.is_symlink(),str(p)
    b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
m=read('saved-manifest.json')
assert {str(p.relative_to(HERE)) for p in HERE.rglob('*') if p.is_file()}==set(m['files'])|{'README.md','verify_saved.py','saved-manifest.json'}
for n,h in m['files'].items():assert fp(HERE/n)==h['savedCopy']==h['sourceFingerprint'],n
r=read('run.json');s=read('summary.json')
assert r['status']==s['status']=='PASS' and len(r['inputs'])==915
assert len(r['calls'])==6 and all(c['status']=='PASS' and c['returncode']==0 for c in r['calls'])
assert all((HERE/(c['label']+'.stderr')).read_bytes()==b'' for c in r['calls'])
assert s['new_session_cases']=={'lifetimes':102,'admission_aliases':78,'expired_terminal':3,'master_bits':[8,16,24]}
assert s['no_output_observed']=={'voice_starts':0,'timer_calls':0,'bus_uploads':0}
for n,h in s['own_inputs'].items():assert r['inputs'][n]=={k:h[k] for k in ('bytes','sha256')}
assert read('first-refusals/preparation-first-refusal.json')['status']=='FAILED_BEFORE_COMPILER_OR_EXECUTABLE'
assert (HERE/'first-refusals/second-run.stderr').stat().st_size>0
assert read('first-fixture/run.json')['status']=='FAIL'
assert (HERE/'first-fixture/session-run.stderr').stat().st_size>0
assert b'EDITOR PREPARE SESSION PASS:102' in (HERE/'session-run.stdout').read_bytes()
assert b'EDITOR ESTABLISH PASS:27' in (HERE/'establish-run.stdout').read_bytes()
assert b'ESTABLISHED OWNER PASS:66' in (HERE/'established-run.stdout').read_bytes()
corrected=read('post-format-host/run.json')
assert corrected['status']=='PASS' and len(corrected['inputs'])==915 and len(corrected['calls'])==6
assert corrected['head']=='d57e178683c3350a0618a1e02cf31d613dd2387d'
for c in corrected['calls']:
    assert c['status']=='PASS' and c['returncode']==0
    assert not (HERE/'post-format-host'/(c['label']+'.stderr')).read_bytes()
assert corrected['inputs']['tests/editor_mixed_prepare_session_test.c']['sha256']!=r['inputs']['tests/editor_mixed_prepare_session_test.c']['sha256']
assert (HERE/'post-format-host/session-run.stdout').read_bytes()==(HERE/'session-run.stdout').read_bytes()
print('PASS saved host preparation-session evidence only; no producer or target replay')
