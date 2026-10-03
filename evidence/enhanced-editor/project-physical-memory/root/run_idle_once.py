import hashlib,json,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent;out=ROOT/('idle-verification-'+str(time.time_ns()))
args=['/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra/.venv/bin/python','-I',str(ROOT/'verify_idle_return.py'),'--original',str(ROOT/'original-processes.json'),'--out',str(out)]
r={'passed':False,'scope':'Separate externally bounded locked read-only original030 idle check','arguments':args,'timeout_seconds':60};start=time.monotonic()
with (ROOT/'idle-execution-claim.json').open('x') as f:f.write(json.dumps(r,indent=2)+'\n')
try:
 p=subprocess.run(args,capture_output=True,text=True,timeout=60);(ROOT/'idle-execution.stdout').write_text(p.stdout);(ROOT/'idle-execution.stderr').write_text(p.stderr);r['returncode']=p.returncode
 if p.returncode:raise RuntimeError('Separate original return check failed; retain reservation/HOLD')
 data=(out/'result.json').read_bytes();result=json.loads(data);r['result']={'path':str(out/'result.json'),'sha256':hashlib.sha256(data).hexdigest()}
 if not result['passed'] or len(result['observations'])!=11:raise RuntimeError('Separate check incomplete')
 r['passed']=True
except BaseException as e:r['error']=str(e);raise
finally:
 r['elapsed_seconds']=time.monotonic()-start;(ROOT/'idle-invocation-result.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r),flush=True)
