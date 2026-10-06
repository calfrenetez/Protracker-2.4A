"""Saved evidence only. No import of operational scripts or target I/O."""
import hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent
def fp(p):
 assert p.is_file() and not p.is_symlink(),str(p)
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def read(n):return json.loads((HERE/n).read_text())
manifest=read('saved-manifest.json')
assert {p.name for p in HERE.iterdir()}==set(manifest['files'])|{'saved-manifest.json'}
assert 'lease.private.json' not in manifest['files']
for n,h in manifest['files'].items():assert fp(HERE/n)==h,n
scope=read('scope-plan.json');run=read('run-terminal.json');start=read('start-terminal.json')
assert scope['sourceCommit']=='5771d3ce1fcc3dee1967ec0c728e1fa654954bba'
assert scope['candidate']['bytes']==385144 and scope['candidate']['sha256']=='21c5efc7fe5d066f56e81ea8cbcea8bc7e95892e8af0a55608a1410d882d9b1e'
assert start['status']=='PASS_ONE_OWNED_030_START' and start['startCalls']==start['reserveCalls']==1
assert run['status']=='PASS_EXACT_030_SOFTWARE_FIXTURE_SETTLEMENT_PENDING' and run['launches']==1
assert run['elapsedSeconds']<=scope['absoluteWorkloadSeconds']==420
assert (HERE/'fixture.log').read_bytes()==(HERE/'expected-host-markers.log').read_bytes()
assert len((HERE/'fixture.log').read_bytes().splitlines())==3
assert (HERE/'fixture.rc').read_bytes()==b'0\n' and (HERE/'complete.flag').read_bytes()==b'COMPLETE\n'
for n,h in run['downloadedLogs'].items():assert fp(HERE/n)==h
assert run['completeAssertions']['bridgeAliasesAndAdmission']==75
settled=read('settlement-terminal.json')
assert settled['status']=='PASS_COMPLETED_FIXTURE_SETTLEMENT_RELEASE_PENDING'
assert settled['deletions']==settled['forcedKills']==settled['failureTargetRequests']==0
assert len(settled['custody']['files'])==6 and settled['custody']['stageAbsent']
assert read('first-release-refusal.json')['status']=='FAILED_HOST_RELEASE_REFUSAL_PRESERVED'
assert read('release-v2-review.json')['targetRequests']==0
release=read('root-verified-release.json');guard=release['guardAfter']
assert release['status']=='PASS_COMPLETED_DIAGNOSTIC_FIXTURE_VERIFIED_RELEASE'
assert release['changedReleaseOnlyVerifier'] and release['physical']=='NOT_RUN'
assert guard['phase']=='EMPTY' and guard['fence']==68
assert not guard['reservation'] and not guard['inflight'] and not guard['holds'] and not guard['potential_resident']
assert release['independentReadback']['pinnedInputsUnchanged']==275
for n,h in release['records'].items():assert fp(HERE/('release-v2-'+n+'.json'))['sha256']==h
proof=read('release-v2-proof.json');assert proof['fence']==67 and proof['records']==release['records']
assert all(proof['facts'].values()) and proof['externally_verified']
print('PASS saved editor-bridge native evidence only; no producer or target replay')
