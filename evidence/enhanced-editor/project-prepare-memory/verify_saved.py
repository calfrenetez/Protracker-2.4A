from pathlib import Path
import hashlib,json
BASE=Path(__file__).resolve().parent
manifest=json.loads((BASE/'saved-bytes-v1.json').read_text())
for name,pin in manifest['files'].items():
    data=(BASE/name).read_bytes()
    assert len(data)==pin['bytes'] and hashlib.sha256(data).hexdigest()==pin['sha256'],name
host=json.loads((BASE/'host-v1/execution.json').read_text())
native=json.loads((BASE/'native-v1/manifest.json').read_text())
assert host['status']=='PASS_HOST_ONLY' and host['source_stable'] and len(host['calls'])==4
assert native['status']=='PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED' and native['source_stable']
assert len(native['calls'])==48 and len(native['units'])==39 and len(native['dependencies'])==122
assert host['source_before']==host['source_after']==native['source_before']==native['source_after']
assert all(c['returncode']==0 and not c.get('failure') for c in host['calls']+native['calls'])
assert all(g['success'] and g['tests']==1 for g in host['groups'])
for log in BASE.glob('host-v1/*/*.stderr'):assert not log.read_bytes(),str(log)
for log in BASE.glob('native-v1/*.stderr'):assert not log.read_bytes(),str(log)
marker=(BASE/'host-v1/test_sampler_prepare_project/01.stdout').read_text()
assert marker.startswith('PREPARE PROJECT PASS:30')
print('PASS saved-byte audit; candidate/compiler/targets never executed by verifier')
