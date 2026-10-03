import hashlib,json,os,subprocess,time
from pathlib import Path
assert __debug__
root=Path('/private/tmp/protracker-pcm-frame-root-z8bmyumb');helper=Path('/private/tmp/protracker-pcm-frame-output-safety-klk3djw8/qualifier-v3/qualify_pcm_frame_output_safety.py');identity=root/'fresh-identity-next.json';gate=json.loads((root/'take-gate.json').read_bytes())
assert gate['scope']=='One portable PCM fixture only' and gate['root_take_recorded'] is True and gate['shared_holds_cleared'] is True
assert gate['helper_sha256']=='9565cc5a762fffd4f576c2af2ea1dbceaea13a2b846b34bbded7db50ff206f2b'
assert hashlib.sha256(helper.read_bytes()).hexdigest()==gate['helper_sha256']
assert time.time()-gate['take_epoch_seconds']<90
subprocess.run(['/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra/.venv/bin/python',str(root/'capture_identity_next.py')],check=True,timeout=10)
identity_sha=hashlib.sha256(identity.read_bytes()).hexdigest()
claim=root/'native-execution-next-claim.json';fd=os.open(claim,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
with os.fdopen(fd,'w') as f:json.dump({'scope':gate['scope'],'claimed_ns':time.time_ns(),'retry_allowed':False},f)
argv=['/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra/.venv/bin/python',str(helper),'/private/tmp/protracker-pcm-frame-output-safety-klk3djw8/native-final','--repo','/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A','--identity',str(identity),'--identity-sha256',identity_sha]
started=time.monotonic();report={'scope':'Root external250s watchdog for one portable native fixture; no automatic retry/cleanup/release','argv':argv,'passed':False,'outer_bound_s':250,'gate':gate}
try:
 with (root/'native-execution-next.log').open('x') as log:r=subprocess.run(argv,stdout=log,stderr=subprocess.STDOUT,timeout=250)
 report['returncode']=r.returncode;report['passed']=r.returncode==0
except Exception as e:report['error']=str(e)
finally:
 report['elapsed_s']=time.monotonic()-started;(root/'native-execution-next.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
assert report['passed']
