"""Read archived evidence bytes only; no extraction, compiler, product or target run."""
import argparse,ast,hashlib,json,re,struct,tarfile
from pathlib import Path,PurePosixPath
assert __debug__
BASE='283aa0c451e18a2aaacffebaf65f4a4790e5605a'
OWNED=['src/editor/paula_readers_song.c','src/editor/paula_readers_song.h','tests/paula_readers_song_test.c','tests/test_paula_readers_song.py']
MODULES=['test_paula_readers_song','test_sampler_paula_readers_boundary','test_paula_preflight_startup']
MARKERS=['PAULA READERS SONG PASS: exact whole-song schedule, independent actual domains and immutable retained masters; host software only','PAULA READERS SONG FAULTS PASS: explicit acceptance, independent fault drain and original deadlines; host software only']
def sha(b):return hashlib.sha256(b).hexdigest()
def safe(n):
 p=PurePosixPath(n);return bool(n) and not p.is_absolute() and p.as_posix()==n and '\\' not in n and all(x not in ('','.','..') for x in n.split('/'))
def enabled(argv):return not any(a.startswith('-DNDEBUG') or (a=='-D' and i+1<len(argv) and argv[i+1].split('=',1)[0]=='NDEBUG') for i,a in enumerate(argv))
def main():
 ap=argparse.ArgumentParser(__doc__);ap.add_argument('directory',nargs='?',type=Path,default=Path(__file__).resolve().parent);d=ap.parse_args().directory
 m=json.loads((d/'manifest.json').read_bytes());payload=(d/'payload.tar.gz').read_bytes();assert m['base_commit']==BASE and m['payload']=={'bytes':len(payload),'sha256':sha(payload)}
 rows=m['files'];expected={f['path']:f for f in rows};assert len(rows)==len(expected) and all(safe(n) for n in expected)
 data={}
 with tarfile.open(d/'payload.tar.gz','r:gz') as tf:
  for f in tf:
   assert safe(f.name) and f.isfile() and f.name in expected and f.name not in data
   b=tf.extractfile(f).read();assert len(b)==f.size==expected[f.name]['bytes'] and sha(b)==expected[f.name]['sha256'];data[f.name]=b
 assert set(data)==set(expected)
 def load(n):return json.loads(data[n])
 def bind(f,n):assert len(data[n])==f['bytes'] and sha(data[n])==f['sha256']
 maps={i:load('records/frozen-source-host-v%d.json'%i)['source'] for i in range(1,6)}
 assert all(len(v)==8115 for v in maps.values())
 assert maps[5]==load('records/frozen-source-native-v5.json')['files']
 baseline=load('records/baseline-manifest.json');assert baseline['base_commit']==BASE and len(baseline['files'])==8111
 assert all(maps[5][n]==v['sha256'] for n,v in baseline['files'].items())
 assert set(maps[5])-set(baseline['files'])==set(OWNED)
 for n in OWNED:assert sha(data['source/'+n])==maps[5][n]
 for n,b in data.items():
  if n.startswith('source/'):assert sha(b)==maps[5][n[7:]]
 for version in range(1,5):
  for n in OWNED:assert sha(data['failed-source/v%d/'%version+n])==maps[version][n]
 r=load('proof/host-final-v5/host-checks.json');assert r['passed'] and r['modules']==MODULES and [g['module'] for g in r['groups']]==MODULES
 assert all(g['passed'] and g['tests']>0 and not g['failures'] and not g['errors'] for g in r['groups'])
 assert r['source_files_before']==r['source_files_after']==8115 and r['export_tree_sha256']==sha(json.dumps(maps[5],sort_keys=True,separators=(',',':')).encode())
 pins=load('preparation/host-tools-v1/toolchain-pins.json')['host'];assert r['compiler']==pins['compiler'] and r['python']==pins['python'] and r['sanitizer_runtime_inputs']==pins['runtime_inputs'] and r['pinned_sdk_inputs']==pins['sdk_inputs']
 queries=0
 for c in r['commands']:
  assert c['stage']=='complete' and c['product_returncode']==0 and enabled(c['executed_argv'])
  bind(c['output_log'],'proof/host-final-v5/'+Path(c['output_log']['path']).name)
  for q in c['dependency_commands']:
   queries+=1;assert q['returncode']==0 and '-M' in q['argv'] and '-MM' not in q['argv'] and '-fsanitize=address,undefined' in q['argv'] and enabled(q['argv'])
   bind(q['output'],'proof/host-final-v5/'+Path(q['output']['path']).name)
  for n,h in c['dependencies']['canonical'].items():assert maps[5][n]==h==sha(data['source/'+n])
  for role in ['produced_binary','input_binary','produced_object']:
   if role in c:bind(c[role],'proof/host-final-v5/products/'+Path(c[role]['archive_path']).name)
 for f in r['products']:bind(f,'proof/host-final-v5/products/'+Path(f['archive_path']).name)
 for marker in MARKERS:
  calls=[c for c in r['commands'] if c['group']=='test_paula_readers_song' and 'input_binary' in c];assert len(calls)==1
  assert data['proof/host-final-v5/'+Path(calls[0]['output_log']['path']).name].count(marker.encode())==1
 for version in range(1,5):
  f=load('proof/host-failed-v%d/host-checks.json'%version);assert not f['passed'] and len(f['commands'])==2 and len(f['groups'])==1 and not f['groups'][0]['passed']
  assert f['commands'][0]['product_returncode']==0 and f['commands'][1]['product_returncode']==-6 and f['source_files_before']==f['source_files_after']==8115
  assert f['export_tree_sha256']==sha(json.dumps(maps[version],sort_keys=True,separators=(',',':')).encode())
  for c in f['commands']:
   bind(c['output_log'],'proof/host-failed-v%d/'%version+Path(c['output_log']['path']).name)
   for n,h in c['dependencies']['canonical'].items():assert maps[version][n]==h
   for q in c['dependency_commands']:
    assert q['returncode']==0 and '-M' in q['argv'] and '-MM' not in q['argv'];bind(q['output'],'proof/host-failed-v%d/'%version+Path(q['output']['path']).name)
   for role in ['produced_binary','input_binary']:
    if role in c:bind(c[role],'proof/host-failed-v%d/products/'%version+Path(c[role]['archive_path']).name)
 n=load('proof/native-final-v5/manifest.json');assert n['passed'] and n['native_NOT_RUN'] and n['base_commit']==BASE and n['source_files_before']==n['source_files_after']==8115
 contract=load('preparation/native-preparation/PTPaulaReadersSongTest-contract-v2.json');target=n['targets'][contract['target']];assert len(n['targets'])==1 and target['source_inputs']==contract['ordered_sources'] and len(target['source_inputs'])==29
 np=load('preparation/native-preparation/toolchain-pins.json');assert n['compiler']==np['native']['compiler'] and n['python']==np['host']['python'] and n['runtime_inputs']==np['native']['runtime_inputs'] and len(n['runtime_inputs'])==7
 flag_nodes=[node.value for node in ast.parse(data['preparation/native-preparation/build_song_readers_faults_portability_v2.py']).body if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='FLAGS' for t in node.targets)]
 assert len(flag_nodes)==1 and n['flags']==ast.literal_eval(flag_nodes[0])
 assert n['source_fingerprint']==sha(json.dumps(maps[5],sort_keys=True,separators=(',',':')).encode()) and n['base_tree']==m['base_tree']==baseline['base_tree']
 assert enabled(n['flags']) and '-m68000' in n['flags'] and '-msoft-float' in n['flags']
 for c in n['commands']:
  assert c['returncode']==0;bind(c['log'],'proof/native-final-v5/'+Path(c['log']['path']).name)
 order=[c['argv'][-1] for c in n['commands'] if '-M' in c['argv']];assert order==contract['ordered_sources']
 for rel,h in target['dependencies'].items():assert maps[5][rel]==h==sha(data['source/'+rel])
 for role in ['binary','preprocessed_fixture','build_log']:bind(target[role],'proof/native-final-v5/'+Path(target[role]['path']).name)
 binary=data['proof/native-final-v5/'+contract['target']];cpp=data['proof/native-final-v5/preprocessed-fixture.log'];assert struct.unpack('>I',binary[:4])[0]==0x3f3 and cpp.count(b'__assert_func (')>=2
 assert all(binary.count(s.encode())==cpp.count(s.encode())==1 for s in MARKERS)
 host_audit=load('records/root-host-audit-final-v5.json');native_audit=load('records/root-native-audit-final-v5.json')
 assert host_audit['passed'] and native_audit['passed'] and len(native_audit['products'])==1
 bind(host_audit['report'],'proof/host-final-v5/host-checks.json');bind(native_audit['products'][0]['report'],'proof/native-final-v5/manifest.json')
 bind(host_audit['source_manifest'],'records/frozen-source-host-v5.json');bind(native_audit['source_manifest'],'records/frozen-source-native-v5.json')
 bind(r['source_manifest'],'records/frozen-source-host-v5.json');bind(r['pins'],'preparation/host-tools-v1/toolchain-pins.json')
 for role,path in [('source_manifest','records/frozen-source-native-v5.json'),('pins','preparation/native-preparation/toolchain-pins.json'),('fixture_contract','preparation/native-preparation/PTPaulaReadersSongTest-contract-v2.json'),('builder','preparation/native-preparation/build_song_readers_faults_portability_v2.py'),('compile_commands','proof/native-final-v5/compile-commands.json')]:bind(n[role],path)
 for module,fixture,prefix in [('test_paula_preflight_startup','tests/paula_preflight_startup_test.c','PAULA STARTUP'),('test_sampler_paula_readers_boundary','tests/sampler_paula_readers_boundary_test.c','PAULA READERS BOUNDARY')]:
  literals=[s for s in re.findall(r'puts\("([^"\n]+)"\)',data['source/'+fixture].decode()) if s.startswith(prefix) and ' PASS:' in s];assert len(literals)==1
  calls=[c for c in r['commands'] if c['group']==module and 'input_binary' in c];assert len(calls)==1
  assert data['proof/host-final-v5/'+Path(calls[0]['output_log']['path']).name].count(literals[0].encode())==1
 print(json.dumps({'passed':True,'members':len(data),'source_files':8115,'archived_source_paths':sum(n.startswith('source/') for n in data),'host_groups':3,'host_commands_RC0':len(r['commands']),'host_full_M':queries,'native_compiler_commands_RC0':len(n['commands']),'native_full_M':len(order),'four_failed_host_versions_preserved':True,'scope':'Archived saved-byte checks only; recorded host PASS/compiler-only proof. Full compiler/dependency edges bound to root audits, external SDK/runtime bytes represented by saved pins. Emulator/physical/native runtime/placement/timing/audio NOT_RUN'},indent=2))
if __name__=='__main__':main()
