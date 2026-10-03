import hashlib,json,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent
PREP=Path('/private/tmp/protracker-project-physical-prepared-VEd4D7')
GATE=ROOT/'gate.json'
read=GATE.read_bytes();gate=json.loads(read)
sha=hashlib.sha256(read).hexdigest()
args=['/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra/.venv/bin/python','-I',str(PREP/'physical_project_stream.py'),'--execute-approved-single-case','--prepared-manifest-sha256','eba2a65de4bc29e0c3fd4b5d568de48c33f6ce9ef043e38face9efda15e269bf','--coordination-gate',str(GATE),'--coordination-gate-sha256',sha]
r={'scope':'One externally bounded physical fixture invocation, no retries or recovery','arguments':args,'timeout_seconds':310,'gate_sha256':sha,'passed':False};start=time.monotonic()
with (ROOT/'root-execution-claim.json').open('x') as f:f.write(json.dumps({'arguments':args,'gate_sha256':sha,'time_ns':time.time_ns()},indent=2)+'\n')
try:
 p=subprocess.run(args,capture_output=True,text=True,timeout=310)
 (ROOT/'physical-execution.stdout').write_text(p.stdout);(ROOT/'physical-execution.stderr').write_text(p.stderr);r['returncode']=p.returncode
 if p.returncode:raise RuntimeError('Physical helper failed; preserve HOLD and target; no retry')
 claim=json.loads((PREP/'physical-execution-claim.json').read_text());out=Path(claim['out']);data=(out/'result.json').read_bytes();result=json.loads(data);r['native_result']={'path':str(out/'result.json'),'sha256':hashlib.sha256(data).hexdigest()}
 if not result['passed'] or result['recovery_hold'] or not result['devbench_returned'] or not result['physical_independent_absence_passed']:raise RuntimeError('Physical completion/release prerequisites incomplete')
 (ROOT/'original-processes.json').write_text(json.dumps(result['initial_original_idle']['processes'],indent=2)+'\n')
 r['passed']=True
except subprocess.TimeoutExpired as e:
 r['error']='External physical helper deadline expired; retain HOLD/target/files and unknown completion'
 for name,b in [('stdout',e.stdout),('stderr',e.stderr)]:
  if b is not None:(ROOT/('physical-execution.'+name)).write_bytes(b if isinstance(b,bytes) else b.encode())
 raise
except BaseException as e:r['error']=str(e);raise
finally:
 r['elapsed_seconds']=time.monotonic()-start;(ROOT/'physical-invocation-result.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r),flush=True)
