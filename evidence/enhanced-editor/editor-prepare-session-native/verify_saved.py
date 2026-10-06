"""Verify saved bytes and relationships only; never import operational callers."""
import hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent
def fp(p):
    assert p.is_file() and not p.is_symlink(),str(p)
    b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def read(n):return json.loads((HERE/n).read_text())
m=read('saved-manifest.json')
assert {p.name for p in HERE.iterdir()}==set(m['files'])|{'saved-manifest.json'}
assert 'lease.private.json' not in m['files']
for n,h in m['files'].items():assert fp(HERE/n)==h,n
b=read('binding.json');scope=read('scope-plan.json')
assert b['sourceCommit']==scope['sourceCommit']=='86cb65fdef27a19082a4d469acee8aa3bb39e0bb'
assert b['sourceCount']==915 and (b['compiledUnits'],b['compilerDependencies'],b['compilerCalls'])==(82,233,91)
assert b['candidate']=={'bytes':395508,'sha256':'b9b9a71351a012f20b5dc67228604f2f6e02d7dce93d07b2fed28fcb8cf83d10'}
assert read('host-mock-tests.json')['status']=='PASS_ISOLATED_HOST_27_CASES_NO_PRODUCTION_ACTIONS'
assert len(read('host-mock-tests.json')['cases'])==27
assert not any(read('host-mock-tests.json')['effects'].values())
assert not read('independent-final-review.json')['blockers']
start=read('start-terminal.json');run=read('run-terminal.json')
assert start['status']=='PASS_ONE_OWNED_030_START' and start['reserveCalls']==start['startCalls']==1
assert run['status']=='PASS_EXACT_030_SOFTWARE_FIXTURE_SETTLEMENT_PENDING' and run['launches']==1
assert run['candidate']==b['candidate'] and run['elapsedSeconds']<=scope['absoluteWorkloadSeconds']==420
assert run['cpu']['model']=='68030' and str(run['cpu']['fpu'])=='0'
assert (run['memory']['chip'],run['memory']['z3'],run['memory']['rtg'])==('2048KB','131072KB','0KB')
assert run['chipset'][1].upper()=='AGA'
assert all(run['dmaAfter']['audio'+str(i)]=='0' for i in range(4))
assert (HERE/'fixture.log').read_bytes()==(HERE/'expected-host-markers.log').read_bytes()
assert fp(HERE/'fixture.log')==b['expectedStdout'] and len((HERE/'fixture.log').read_bytes().splitlines())==3
assert (HERE/'fixture.rc').read_bytes()==b'0\n' and (HERE/'complete.flag').read_bytes()==b'COMPLETE\n'
for n,h in run['downloadedLogs'].items():assert fp(HERE/n)==h
c=run['completeAssertions']
assert (c['editorLegacy'],c['editorCheckedLifecycle'],c['wholeControlAliases'],c['sessionLifecycle'],c['sessionAdmissionAliases'],c['sessionExpiredTerminal'])==(15,39,18,102,78,3)
settled=read('settlement-terminal.json')
assert settled['status']=='PASS_COMPLETED_FIXTURE_SETTLEMENT_RELEASE_PENDING'
assert settled['deletions']==settled['forcedKills']==settled['failureTargetRequests']==0
assert len(settled['custody']['files'])==6 and settled['custody']['stageAbsent']
release=read('root-verified-release.json');g=release['guardAfter']
assert release['status']=='PASS_COMPLETED_DIAGNOSTIC_FIXTURE_VERIFIED_RELEASE' and release['physical']=='NOT_RUN'
assert g['phase']=='EMPTY' and g['fence']==108
assert not g['reservation'] and not g['inflight'] and not g['holds'] and not g['potential_resident']
assert release['independentReadback']['pinnedInputsUnchanged']==2386
for n,h in release['records'].items():assert fp(HERE/('release-'+n+'.json'))['sha256']==h
proof=read('release-proof.json');assert proof['records']==release['records'] and proof['fence']==107
assert proof['externally_verified'] and all(proof['facts'].values())
archive=read('release-archive-receipt.json')
assert archive['status']=='PASS_ROOT_PRIVATE_RELEASE_ARCHIVE_BYTE_READBACK'
assert archive['privateEnvelopeNotPublished'] and archive['canonicalInternalDigestVerified']
assert archive['file']==next(x for x in g['archives'] if x['fence']==108)
print('PASS saved source86 preparation-session native fixture and release; no target replay')
