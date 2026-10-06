"""Saved public HOST evidence verification only; no compiler/product/target/Git."""
from pathlib import Path
import hashlib,json
BASE=Path(__file__).resolve().parent
REPO=BASE.parents[2]
def pin(p):
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def main():
 inventory=json.loads((BASE/'files.json').read_text())
 actual={str(p.relative_to(BASE)):pin(p) for p in sorted(BASE.rglob('*')) if p.is_file() and p.name!='files.json'}
 assert actual==inventory, 'public packet changed'
 ledger=json.loads((BASE/'attempts.json').read_text());current=json.loads((BASE/'current.json').read_text())
 assert set(current)=={'factory','activation','combined'}
 checks=0
 for group,v in current.items():
  chosen=[r for r in ledger if r['group']==group and r['current']];assert len(chosen)==1
  r=chosen[0];assert r['attempt']==v['attempt'] and r['status']=='PASS'
  folder=BASE/group/r['attempt'];inputs=json.loads((folder/'inputs.json').read_text());overlays=json.loads((folder/'overlays.json').read_text());calls=json.loads((folder/'calls.json').read_text())
  labels=([group+'-compile',group+'-run'] if group=='combined' else [group+'-compile',group+'-run','paired-compile','paired-run','abi2-compile','abi2-run'])
  assert [c['label'] for c in calls]==labels and len(inputs)==v['source_count']
  assert [x for x in calls[0]['argv'] if x.endswith('.c')]==v['ordered_units']
  for c in calls:
   assert c['status']=='PASS' and c['returncode']==0 and not (folder/(c['label']+'.stderr')).read_bytes();checks+=1
  stdout=(folder/(group+'-run.stdout')).read_bytes();assert pin(folder/(group+'-run.stdout'))==v['stdout'] and stdout.endswith(b'\n') and b'SOFTWARE_ONLY' in stdout
  for name,value in overlays.items():
   p=REPO/name;assert pin(p)=={k:value[k] for k in ('bytes','sha256')} and p.stat().st_mode&0o777==value['mode']
 assert len([r for r in ledger if r['status']!='PASS'])==14
 print(json.dumps({'status':'PASS_SAVED_PUBLIC_HOST_PACKET','files':len(actual),'attempts':len(ledger),'successful_current_calls':checks,'product_or_target_execution':'NONE','limits':'Saved HOST software evidence and current overlays; no live Git/ownership/native/device/timing/listening claim'}))
if __name__=='__main__':main()
