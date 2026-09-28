from pathlib import Path
import concurrent.futures,json,os,subprocess,sys,time
root=Path(__file__).resolve().parents[2]
candidate=root/'build/dev/studio-prefill-candidate'
out=root/'build/dev/studio-prefill-host-suite';out.mkdir(exist_ok=False)
modules=sorted(p.stem for p in (candidate/'tests').glob('test_*.py'))
start=time.monotonic();results=[]
def run(module):
 t=time.monotonic();testroot=root if module in ('test_editor_invert_reference','test_editor_wavetable') else candidate;env=dict(os.environ,PYTHONPATH=str(testroot/'tests'))
 with (out/(module+'.log')).open('w') as log:
  try:r=subprocess.run([sys.executable,'-m','unittest',module],cwd=testroot,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=180);rc=r.returncode
  except subprocess.TimeoutExpired:rc='timeout'
 return {'module':module,'returncode':rc,'seconds':round(time.monotonic()-t,3),'context':'Git isolation helper' if testroot==root else 'indexed export'}
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
 for f in concurrent.futures.as_completed([pool.submit(run,m) for m in modules]):
  r=f.result();results.append(r)
  summary={'source_tree':(candidate/'source-tree.txt').read_text().strip(),'modules':len(modules),'completed':len(results),'failed':sum(r['returncode']!=0 for r in results),'seconds':round(time.monotonic()-start,3),'results':sorted(results,key=lambda x:x['module'])}
  (out/'result.json').write_text(json.dumps(summary,indent=2)+'\n')
  print(len(results),'/',len(modules),r,flush=True)
sys.exit(bool(summary['failed']))
