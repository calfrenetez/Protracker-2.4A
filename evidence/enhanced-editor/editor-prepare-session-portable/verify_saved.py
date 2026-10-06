"""Saved compiler evidence only, with no compiler/product/target execution."""
import hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent
def read(n):return json.loads((HERE/n).read_text())
def fp(p):
    assert p.is_file() and not p.is_symlink(),str(p)
    b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
saved=read('saved-manifest.json')
assert {str(p.relative_to(HERE)) for p in HERE.rglob('*') if p.is_file()}==set(saved['files'])|{'saved-manifest.json','README.md','verify_saved.py'}
for n,h in saved['files'].items():assert fp(HERE/n)==h['savedCopy']==h['sourceFingerprint'],n
m=read('manifest.json');first=read('first-portable-failure/manifest.json')
assert m['status']=='PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED'
assert all(m[n]=='NEVER_EXECUTED' for n in ('target_execution','emulator_execution','physical_execution'))
assert m['source_count']==len(m['source_before'])==915 and m['unit_count']==82
assert m['source_before']==m['source_after'] and m['source_stable']
assert len(m['dependencies'])==233 and len(m['tools'])==5 and len(m['runtime_inputs'])==7
assert len(m['calls'])==91 and all(c['status']=='PASS' and c['returncode']==0 and c['stderr']['bytes']==0 for c in m['calls'])
assert all(c['timeout_seconds']==120 for c in m['calls']) and m['bounds']['overall_seconds']==600
assert {'-m68000','-msoft-float','-mcrt=nix20','-UNDEBUG'}<=set(m['flags'])
assert m['product']['bytes']==395508 and m['product']['sha256']=='b9b9a71351a012f20b5dc67228604f2f6e02d7dce93d07b2fed28fcb8cf83d10'
assert m['product']['execution']=='NEVER_EXECUTED'
assert fp(HERE/'expected-native.stdout')=={k:m['expected_stdout'][k] for k in ('bytes','sha256')}
assert first['status']=='FIRST_FAILURE_RETAINED' and 'product' not in first
assert first['calls'][-1]['returncode']!=0
assert b'misleading-indentation' in (HERE/'first-portable-failure/compile-link.stderr').read_bytes()
assert read('first-portable-failure/first-failure-custody.json')['manifest']==fp(HERE/'first-portable-failure/manifest.json')
print('PASS saved portable session compiler evidence; products never executed')
