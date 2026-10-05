"""Saved bytes only: no extraction, helper import, compiler or executable run."""
import hashlib,json,re,struct,tarfile
from pathlib import Path
def sha(b):return hashlib.sha256(b).hexdigest()
r=Path(__file__).resolve().parent;m=json.loads((r/'manifest.json').read_bytes());p=r/'payload.tar.gz'
assert p.stat().st_size==m['payload']['bytes'] and sha(p.read_bytes())==m['payload']['sha256']
d={}
with tarfile.open(p,'r:gz') as tar:
 for item in tar.getmembers():
  assert item.isfile() and item.name not in d and not Path(item.name).is_absolute() and '..' not in Path(item.name).parts
  b=tar.extractfile(item).read();x=m['members'][item.name]
  assert len(b)==x['bytes'] and sha(b)==x['sha256'] and item.mode==x['mode'];d[item.name]=b
assert set(d)==set(m['members'])
def load(n):return json.loads(d[n])
def mapped(x):
 b=d[m['path_members'][x['path']]];assert len(b)==x['bytes'] and sha(b)==x['sha256'];return b
f=load(m['source_manifest']);n=load(m['native_proof']);h=load(m['prior_host_proof'])
assert n['passed'] and n['native_NOT_RUN'] and len(n['commands'])==42 and all(c['returncode']==0 for c in n['commands'])
assert n['source_files_before']==n['source_files_after']==8142
assert h['passed'] and len(h['groups'])==7 and len(h['commands'])==14 and all(c['product_returncode']==0 for c in h['commands'])
t=n['targets']['PTExecReadersBackendRegistrationTest']
for rel,name in m['source_members'].items():assert sha(d[name])==f['files'][rel]
for rel,v in t['dependencies'].items():assert sha(d[m['source_members'][rel]])==v==f['files'][rel]
for rel,v in h['canonical_dependencies'].items():assert sha(d[m['source_members'][rel]])==v==f['files'][rel]
assert len(t['source_inputs'])==31 and len(t['dependencies'])==79 and len(t['external_sdk_dependencies'])==49
assert len(n['runtime_inputs'])==7
for c in n['commands']:mapped(c['log'])
for key in ['builder','pins','fixture_contract','source_manifest','compile_commands']:mapped(n[key])
e=load('controls/native-execution-v1.json');assert e['driver_rc']==0 and len(e['durable_copies'])==45
for pair in e['durable_copies']:
 assert mapped(pair['original'])==mapped(pair['durable'])
 assert m['members'][m['path_members'][pair['durable']['path']]]['mode']==pair['durable']['mode']==pair['original']['mode']
b=mapped(t['binary']);cpp=mapped(t['preprocessed_fixture'])
assert struct.unpack('>I',b[:4])[0]==0x3f3
assert '__assert_func' in t['last_assert_macro']
assert len(re.findall(rb'\b__assert_func\s*\(',cpp))==133
assert len(re.findall(rb'^#define assert\([^\n]*',cpp,re.M))==2
assert len(re.findall(rb'void __assert_func\s*\(',cpp))==2
assert len(re.findall(rb'__assert_func\s*\(\s*"[^"]+"\s*,',re.sub(rb'^#.*\n',b'',cpp,flags=re.M)))==129
for marker,count in n['HUNK_required_marker_counts'].items():assert count==1 and b.count(marker.encode())==1
for marker,count in n['CPP_required_marker_counts'].items():assert count==1 and cpp.count(marker.encode())==1
for name in ['final-frozen-source-review-v1.json','root-saved-native-review-v1.json','independent-saved-native-review-v1.json','final-documentation-review-v1.json']:
 assert load('controls/'+name)['status']=='CLEAR'
for name in ['native_execution','emulator','physical']:assert m['acceptance'][name]=='NOT_RUN'
assert m['acceptance']['stack']=='UNKNOWN' and m['acceptance']['launcher_65536']=='NOT_CLEARED'
print(json.dumps({'state':'PASS_SAVED_BYTES_ONLY','members':len(d),'sources':len(m['source_members']),'native_build':'PASS','native_execution':'NOT_RUN','emulator':'NOT_RUN','physical':'NOT_RUN','stack':'UNKNOWN'}))
