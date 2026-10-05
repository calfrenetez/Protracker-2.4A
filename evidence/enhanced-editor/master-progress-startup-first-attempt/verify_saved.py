from pathlib import Path
import hashlib,json
BASE=Path(__file__).resolve().parent
m=json.loads((BASE/'saved-bytes-v1.json').read_text())
for name,pin in m['files'].items():
 b=(BASE/name).read_bytes();assert len(b)==pin['bytes'] and hashlib.sha256(b).hexdigest()==pin['sha256'],name
s=json.loads((BASE/'start-terminal.json').read_text());i=json.loads((BASE/'host-only-failure-identity-v1.json').read_text())
assert s['status']=='FIRST_FAILURE_RETAINED_NO_RETRY' and s['startCalls']==s['reserveCalls']==1 and s['returncode']==1
assert s['guardBefore']['phase']=='EMPTY' and s['guardBefore']['fence']==34
g=s['guardAfter'];assert g['phase']=='HOLD' and g['fence']==40 and g['inflight'] is None and g['potential_resident']
assert g['reservation']['binding']['sourceCommit']=='120e51d6b90303484f6108a1c6b8ff5f2e6d10cb'
assert s['candidate']['bytes']==292912 and s['candidate']['sha256']=='2c88205c370a3ab8d68a0f5beecc851d0d9e44e9e90a95d590a74185d03348e8'
assert len(i['ownedProcess'])==1 and i['ownedProcess'][0][0]==35970 and i['runtime']['pid']==35970
assert not i['stageExists'] and not i['runTerminalExists']
assert 'GuardRefused: Target tool reported an error' in (BASE/'start.stderr').read_text()
p=json.loads((BASE/'startup-recovery-plan.json').read_text());assert p['approved'] is False and p['pid']==35970 and p['fence']==40
for name,h in json.loads((BASE/'startup-recovery-root-review.json').read_text())['scripts'].items():assert hashlib.sha256((BASE/name).read_bytes()).hexdigest()==h
assert not (BASE/'explicit-startup-recovery-approval.json').exists() and not (BASE/'lease.private.json').exists() and not (BASE/'run-terminal.json').exists()
print('PASS saved first startup failure/prepared recovery; no native execution, approval, release or replay')
