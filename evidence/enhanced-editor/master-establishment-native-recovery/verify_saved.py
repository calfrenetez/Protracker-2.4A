from pathlib import Path
import hashlib,json
BASE=Path(__file__).resolve().parent
def fp(p):
 data=p.read_bytes();return {'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
manifest=json.loads((BASE/'saved-bytes-v1.json').read_text())
assert {p.name for p in BASE.iterdir()}==set(manifest['files'])|{'saved-bytes-v1.json'}
for name,pin in manifest['files'].items():assert fp(BASE/name)==pin,name
approval=json.loads((BASE/'explicit-recovery-approval.json').read_text())
assert approval['approved'] is True and approval['pid']==32716 and approval['fence']==33 and approval['human_response']=='go ahead'
for name,h in approval['scripts'].items():assert fp(BASE/name)['sha256']==h
closed=json.loads((BASE/'recovery-close-terminal.json').read_text())
assert closed['status']=='PASS_OWNED_EMULATOR_RECOVERY_HOLD_RETAINED' and closed['sigtermCalls']==1
assert not closed['forcedKills'] and not closed['guestRequests'] and not closed['deletions'] and not closed['guardReleaseCalls']
assert closed['guardBefore']==closed['guardAfter'] and closed['guardAfter']['phase']=='HOLD' and closed['guardAfter']['fence']==33
for name in ('fixture.log','fixture.rc','complete.flag','launcher.log'):assert fp(BASE/('late-'+name))==closed['custody']['files'][name]
assert (BASE/'late-fixture.rc').read_bytes()==b'0\n' and (BASE/'late-complete.flag').read_bytes()==b'COMPLETE\n'
release=json.loads((BASE/'root-verified-release.json').read_text());proof=json.loads((BASE/'release-proof.json').read_text())
assert release['status']=='PASS_EXACT_OWNED_RECOVERY_VERIFIED_RELEASE' and release['release']['released'] is True
assert release['originalNativeAttempt']=='FAILED, not retried or promoted' and release['physical']=='NOT_RUN'
g=release['guardAfter'];assert g['phase']=='EMPTY' and g['fence']==34 and not g['reservation'] and not g['inflight'] and not g['holds'] and not g['potential_resident']
assert all(release['independentReadback'][x] is True for x in ('allEmulatorsAbsent','port2345ListenerAbsent','DevBenchDisconnected','exactStageAbsentAllCustodyBytesPreserved'))
assert release['independentReadback']['originalInputsUnchanged']==11
assert proof['fence']==33 and proof['externally_verified'] is True and proof['records']==release['records']
for name,h in proof['records'].items():assert fp(BASE/('recovery-'+name+'.json'))['sha256']==h and proof['facts'][name] is True
assert json.loads((BASE/'recovery-release-coordination-v1.json').read_text())['revision']==18
first=BASE.parent/'master-establishment-native-first-attempt'
assert fp(first/'run-terminal.json')==closed['originalFailedResult']
assert not (first/'failure-fixture.log').read_bytes()
assert 'lease.private.json' not in manifest['files'] and 'PTMasterEstablishTest' not in manifest['files']
print('PASS exact saved recovery/release; first native failure preserved, no retry or physical acceptance')
