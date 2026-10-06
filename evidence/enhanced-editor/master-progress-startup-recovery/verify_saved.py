"""Saved-byte/readback checks only: no recovery or target replay."""
from pathlib import Path
import hashlib,json
BASE=Path(__file__).resolve().parent
for name,pin in json.loads((BASE/'saved-bytes.json').read_text())['files'].items():
    data=(BASE/name).read_bytes();assert len(data)==pin['bytes'] and hashlib.sha256(data).hexdigest()==pin['sha256'],name
r=json.loads((BASE/'startup-recovery-terminal.json').read_text())
assert r['status']=='PASS_FAILED_START_RECOVERY_HOLD_RETAINED' and r['sigtermCalls']==1
assert not any(r[k] for k in ('forcedKills','guestRequests','deletions','releaseCalls'))
assert r['guardBefore']==r['guardAfter'] and r['guardAfter']['fence']==40
v=json.loads((BASE/'startup-root-verified-release.json').read_text());g=v['guardAfter']
assert v['status']=='PASS_EXACT_FAILED_START_RECOVERY_RELEASE' and v['release']['released']
assert g['phase']=='EMPTY' and g['fence']==41 and not any(g[k] for k in ('reservation','holds','inflight','potential_resident'))
assert v['physical']=='NOT_RUN' and all(v['independentReadback'].values())
for name,digest in v['records'].items():assert hashlib.sha256((BASE/('startup-recovery-'+name+'.json')).read_bytes()).hexdigest()==digest
print('PASS saved recovery and independent release; original startup failure preserved; no replay')
