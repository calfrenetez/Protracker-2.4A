import json,subprocess,time
from pathlib import Path
root=Path('/private/tmp/protracker-pcm-frame-root-z8bmyumb');started=time.monotonic();r={'scope':'External60s watchdog for separate read-only idle verification','passed':False,'outer_bound_s':60}
try:
 with (root/'separate-idle-execution.log').open('x') as log:p=subprocess.run(['/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra/.venv/bin/python',str(root/'verify_native_return.py')],stdout=log,stderr=subprocess.STDOUT,timeout=60)
 r['returncode']=p.returncode;r['passed']=p.returncode==0
except Exception as e:r['error']=str(e)
finally:r['elapsed_s']=time.monotonic()-started;(root/'separate-idle-execution.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
assert r['passed']
