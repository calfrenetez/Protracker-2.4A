from pathlib import Path
import hashlib,json
BASE=Path(__file__).resolve().parent
manifest=json.loads((BASE/'saved-bytes-v1.json').read_text())
for name,pin in manifest['files'].items():
 data=(BASE/name).read_bytes();assert len(data)==pin['bytes'] and hashlib.sha256(data).hexdigest()==pin['sha256'],name
start=json.loads((BASE/'start-terminal.json').read_text());run=json.loads((BASE/'run-terminal.json').read_text())
assert start['status']=='PASS_ONE_OWNED_030_START' and start['startCalls']==start['reserveCalls']==1
assert run['status']=='FIRST_FAILURE_HOLD_NO_RETRY' and run['launches']==1
assert run['candidate']['bytes']==291740 and run['candidate']['sha256']=='06cb163c7a5fe0c16c92f5e328267ce731d77f3098ecf6a60df3d20b81e43d4a'
assert run['guardAfter']['phase']=='HOLD' and run['guardAfter']['fence']==33 and run['guardAfter']['potential_resident'] and run['guardAfter']['inflight'] is None
assert 'launchReply' in run and run['launchReply']=='[OK] (no output)' and 'downloadedLogs' not in run
assert not (BASE/'failure-fixture.log').read_bytes() and (BASE/'failure-launcher.log').read_bytes()==b'[CLI 5]\n'
assert json.loads((BASE/'recovery-plan.json').read_text())['approval_pending'] is True
print('PASS exact saved first failure; no native acceptance, release, retry or target action')
