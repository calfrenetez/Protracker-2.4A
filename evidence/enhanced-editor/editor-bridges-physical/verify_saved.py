"""Verify saved evidence only; no operational imports or target requests."""
import hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent
def fp(p):
    assert p.is_file() and not p.is_symlink(),str(p)
    b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def read(n):return json.loads((HERE/n).read_text())
manifest=read('saved-manifest.json')
assert {p.name for p in HERE.iterdir()}==set(manifest['files'])|{'saved-manifest.json','README.md','verify_saved.py'}
for n,h in manifest['files'].items():
    assert 'private.json' not in n
    assert fp(HERE/n)==h['savedCopy']==h['sourceFingerprint'],n
run=read('physical-run.json');settled=read('physical-settlement.json')
assert run['status']=='PASS_EXACT_PHYSICAL_SOFTWARE_FIXTURE_SETTLEMENT_PENDING'
assert run['launches']==run['connectCalls']==1 and run['target']=='real-a1200'
assert run['elapsedSeconds']<=420 and run['observations']==25
assert run['files']['PTEditorBridgesTest']=={'bytes':385144,'sha256':'21c5efc7fe5d066f56e81ea8cbcea8bc7e95892e8af0a55608a1410d882d9b1e'}
assert run['files']['fixture.log']=={'bytes':624,'sha256':'80a8a9731ef96568d0356de80dfdf5299ee7bef777cc0a8e7a5d038ead11d80a'}
for n in ('Run-once','fixture.log','fixture.rc','complete.flag','launcher.log'):
    assert fp(HERE/n)==run['files'][n],n
assert (HERE/'fixture.rc').read_bytes()==b'0\n'
assert (HERE/'complete.flag').read_bytes()==b'COMPLETE\n'
assert len((HERE/'fixture.log').read_bytes().splitlines())==3
assert settled['status']=='PASS_COMPLETED_PHYSICAL_FIXTURE_FILES_ABSENT'
physical=read('physical-verified-release.json')
assert physical['status']=='PASS_PHYSICAL_SCOPE_RELEASE_ORIGINAL030_RETURN_PENDING'
assert not physical['fullWindowReleased'] and physical['guardAfter']['phase']=='EMPTY'
assert read('physical-release-restoration.json')['fullSharedRestoration'] is False
start=read('return-start.json');idle=read('return-idle.json')
assert start['status']=='PASS_ONE_ORIGINAL030_RETURN_START_IDLE_CHECK_PENDING'
assert start['reserveCalls']==start['startCalls']==1 and start['returncode']==0
assert idle['status']=='PASS_ORIGINAL030_IDLE_RETURN_AND_NORMAL_STOP_RELEASE_PENDING'
assert idle['normalStop'] and idle['forcedKills']==idle['candidateLaunches']==0
assert idle['cpu']['model']=='68030' and str(idle['cpu']['fpu'])=='0'
assert idle['memory']['chip']=='2048KB' and idle['memory']['z3']=='131072KB'
assert idle['chipset'][1].upper()=='AGA' and all(idle['dma']['audio'+str(i)]=='0' for i in range(4))
release=read('root-verified-release.json');guard=release['guardAfter']
assert release['status']=='PASS_EXACT_PHYSICAL_SOFTWARE_FIXTURE_FULL_VERIFIED_RELEASE'
assert release['fullWindowReleased'] and release['overallRestoration']=='COMPLETE_ORIGINAL030_RETURN_VERIFIED'
assert guard['phase']=='EMPTY' and guard['fence']==100
assert not guard['reservation'] and not guard['inflight'] and not guard['holds'] and not guard['potential_resident']
for prefix,r in [('physical-release',physical),('final-release',release)]:
    for kind,h in r['records'].items():assert fp(HERE/(prefix+'-'+kind+'.json'))['sha256']==h
    proof=read(prefix+'-proof.json');assert proof['records']==r['records']
    assert proof['externally_verified'] and all(proof['facts'].values())
assert read('final-release-restoration.json')['original030ReturnObligation']=='COMPLETE'
assert read('final-release-restoration.json')['originalDisconnectedEndpoint']=={'host':'127.0.0.1','port':2345,'connected':False}
assert read('safari-after.json')['status']=='AUTHENTICATED_VIEW_ONLY_VERIFIED'
assert not read('safari-after.json')['controlAcquired'] and not read('safari-after.json')['remoteInput']
print('PASS saved exact physical editor software evidence; no producer or target replay')
