"""Read saved bytes only. No extraction, compiler, test binary or target operation."""
import hashlib,json,re,struct,tarfile
from pathlib import Path
def sha(raw):return hashlib.sha256(raw).hexdigest()
root=Path(__file__).resolve().parent
m=json.loads((root/'manifest.json').read_bytes());payload=root/'payload.tar.gz'
assert len(payload.read_bytes())==m['payload']['bytes'] and sha(payload.read_bytes())==m['payload']['sha256']
data={}
with tarfile.open(payload,'r:gz') as tar:
 members=tar.getmembers();assert len(members)==len(m['members'])
 for item in members:
  assert item.isfile() and item.name not in data and not Path(item.name).is_absolute() and '..' not in Path(item.name).parts
  raw=tar.extractfile(item).read();row=m['members'][item.name]
  assert len(raw)==row['bytes'] and sha(raw)==row['sha256'] and item.mode==row['mode']
  data[item.name]=raw
def load(name):return json.loads(data[name])
def mapped(row):
 name=m['path_members'][row['path']];raw=data[name];assert len(raw)==row['bytes'] and sha(raw)==row['sha256'];return raw
f=load(m['source_manifest']);h=load(m['host_proof']);n=load(m['native_proof'])
assert h['passed'] and len(h['groups'])==7 and all(g['passed'] for g in h['groups']) and len(h['commands'])==14
assert all(c['product_returncode']==0 for c in h['commands'])
assert n['passed'] and n['native_NOT_RUN'] is True and len(n['commands'])==42 and all(c['returncode']==0 for c in n['commands'])
for rel,name in m['source_members'].items():assert sha(data[name])==f['files'][rel]
for rel,digest in h['canonical_dependencies'].items():assert sha(data[m['source_members'][rel]])==digest==f['files'][rel]
t=n['targets']['PTReadersBackendRegistrationTest']
for rel,digest in t['dependencies'].items():assert sha(data[m['source_members'][rel]])==digest==f['files'][rel]
for proof in ['controls/host-execution-v1.json','controls/native-execution-v1.json']:
 e=load(proof);assert e['driver_rc']==0
 for pair in e['durable_copies']:
  assert mapped(pair['original'])==mapped(pair['durable'])
for p in h['products']:
 raw=data[m['path_members'][p['archive_path']]];assert len(raw)==p['bytes'] and sha(raw)==p['sha256']
marker=b'READERS BACKEND REGISTRATION PASS: public opaque bindings; genuine independent software ownership; no activation or hardware timing proof'
binary=mapped(t['binary']);assert struct.unpack('>I',binary[:4])[0]==0x3f3 and binary.count(marker)==1
cpp=mapped(t['preprocessed_fixture']);assert cpp.count(marker)==1 and b'__assert_func' in t['last_assert_macro'].encode()
for name in ['controls/final-frozen-source-review-v1.json','controls/independent-saved-host-review-v1.json','controls/independent-saved-native-review-v1.json','controls/final-documentation-review-v1.json']:assert load(name)['status']=='CLEAR'
assert mapped(h['log']).count(marker)==1
assert all(m['acceptance'][name]=='NOT_RUN' for name in ['native_execution','emulator','physical'])
print(json.dumps({'state':'PASS_SAVED_BYTES_ONLY','members':len(data),'sources':len(m['source_members']),'host':'PASS','native_build':'PASS','native_execution':'NOT_RUN','emulator':'NOT_RUN','physical':'NOT_RUN'}))
