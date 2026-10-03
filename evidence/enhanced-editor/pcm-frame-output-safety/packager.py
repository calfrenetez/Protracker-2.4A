"""Package only frozen private PCM host/build/probe and unexecuted qualifier preparation.

No source mutation, test/build/helper import, target command or repo operation.
Full inventories are retained; only actual execution-source closures are copied.
"""
import argparse,gzip,hashlib,io,json,os,stat,tarfile
from pathlib import Path,PurePosixPath
ROOT=Path('/private/tmp/protracker-pcm-frame-output-safety-klk3djw8')
PINS={
 'baseline-manifest.json':'2b9a35b1fd7e0929ae5f31246deedef99b09e75c8436fb427834746a10eca67c',
 'source-manifest.json':'b88837bae2b7767cced5ad5a6c030306ea4ac2bc4d73e8c6f433dd41ca4d7ddc',
 'host-callers-final/host-checks.json':'aab2332ff0897ae87ac2d51e3b7d86f5381ffa3f4793e51be5ef3ad44ea957d1',
 'host-final/host-checks.json':'f5b83f9cf51fd7aba851a444761c187df2347e28c59bf829b873858e1e222187',
 'native-final/manifest.json':'846a791f9cc4e2d7556bc1c48d58dc2c81a3274e2f8e07de5090c112f98014b5',
 'baseline-probe/result.json':'33044e0256673ee483c8030fd971e1240a417f70c1a2ba5c836c21d6befa0a5f',
 'candidate-probe/result.json':'1bb9cb6e96aef4d0aea0fae83c30d9b80a6b26e474aad979770ec9d8492ff99e',
 'final.patch':'994ba0c414cd7ad6829573789456ead319c4b63bf886daa0f8acc8cb22bf3041',
 'output-probe.c':'87638fbc915b1880ee33bf414794496151432de6a71e6c04b314ebc8e5998288',
 'handoff-complete-v2.json':'65d9687fed2416bd9eb21531722e65b16a80f9b1619ed676c62cbb7be4477730',
 'qualifier/qualify_pcm_frame_output_safety.py':'30d279d5cabe5118ce34cd2955e5a8c5b639ba5bbd8fba118896c4171224cc71',
}
OVERLAYS={'src/core/pcm.c','src/core/pcm.h','tests/pcm_test.c'}
EXTRA_SOURCE={'tests/test_sampler.py','tests/test_paula_preview.py','tests/test_pcm.py','tests/test_filter.py','tools/generate_sinc_kernel.py','tools/build_core_tests.py','tools/shared_infra_render_files.py'}

def sha(data):return hashlib.sha256(data).hexdigest()
def raw_json(path):return json.loads(path.read_bytes())
def json_bytes(value):return (json.dumps(value,indent=2)+'\n').encode()
def regular(path,allow_symlink=False):
 if (path.is_symlink() and not allow_symlink) or not stat.S_ISREG(path.stat().st_mode):raise ValueError('Only genuine regular inputs allowed: '+str(path))
def record(path,allow_symlink=False):
 regular(path,allow_symlink);data=path.read_bytes()
 return {'path':str(path),'bytes':len(data),'sha256':sha(data),'mode':stat.S_IMODE(path.stat().st_mode)}
def require(path,expected,bytes_=None,allow_symlink=False):
 r=record(path,allow_symlink)
 if r['sha256']!=expected or (bytes_ is not None and r['bytes']!=bytes_):raise ValueError('Pinned origin mismatch: '+str(path))
 return r

def snapshot(directory):
 rows={}
 for p in sorted(directory.rglob('*')):
  if p.is_symlink():raise ValueError('Source symlink refused: '+str(p))
  if p.is_file():rows[p.relative_to(directory).as_posix()]=record(p)['sha256']
 return rows

def check_recorded_files(value,source):
 """Rehash exact existing logs/helpers/modules/products/toolchain records.

 Absolute archived products are authoritative for removed host temp binaries.
 Relative C records resolve against their immutable recorded source export.
 """
 if isinstance(value,dict):
  if 'sha256' in value and ('path' in value or 'archive_path' in value):
   p=Path(value.get('archive_path',value.get('path')))
   if not p.is_absolute():p=source/p
   require(p,value['sha256'],value.get('bytes'),allow_symlink=ROOT not in p.parents)
  for x in value.values():check_recorded_files(x,source)
 elif isinstance(value,list):
  for x in value:check_recorded_files(x,source)

def pinned_map(mapping):
 for path,expected in mapping.items():require(Path(path),expected,allow_symlink=True)

def prepare():
 for name,expected in PINS.items():require(ROOT/name,expected)
 b=raw_json(ROOT/'baseline-manifest.json');s=raw_json(ROOT/'source-manifest.json')
 h=raw_json(ROOT/'host-final/host-checks.json');c=raw_json(ROOT/'host-callers-final/host-checks.json');n=raw_json(ROOT/'native-final/manifest.json')
 old=raw_json(ROOT/'baseline-probe/result.json');new=raw_json(ROOT/'candidate-probe/result.json')
 mock=raw_json(ROOT/'qualifier/mock-proof.json');prepared=raw_json(ROOT/'qualifier/prepared.json')
 if b['base_commit']!='8969da594039fa23e43ab1d1ed0907d250b596f3' or b['base_tree']!='c9e522973e2b4b4943eec9b0bfd5639f4eaeadc4':raise ValueError('Wrong immutable base identity')
 if set(s['overlays'])!=OVERLAYS or b['overlays'] or len(b['source'])!=7633 or len(s['source'])!=7633:raise ValueError('Three-overlay7633-source boundary changed')
 if b['git_entries']!=s['git_entries'] or b['dirty_exclusions']!=s['dirty_exclusions'] or len(s['dirty_exclusions']['paths'])!=16:raise ValueError('Baseline metadata/dirty exclusions changed')
 if set(b['source'])!=set(s['source']) or {p for p in s['source'] if b['source'][p]!=s['source'][p]}!=OVERLAYS:raise ValueError('Extra source differences')
 for p in OVERLAYS:
  if s['overlays'][p]['before_sha256']!=b['source'][p] or s['overlays'][p]['after_sha256']!=s['source'][p]:raise ValueError('Overlay before/after hash mismatch')
  require(ROOT/'overlay-final'/p,s['source'][p])
 if snapshot(ROOT/'baseline')!=b['source'] or snapshot(ROOT/'source-final')!=s['source']:raise ValueError('Full immutable source inventory changed')
 require(Path(b['archive']['path']),b['archive']['sha256'],b['archive']['bytes'])
 if not h['passed'] or len(h['groups'])!=2 or len(h['commands'])!=4 or any(c['product_returncode']!=0 for c in h['commands']):raise ValueError('Host proof not exact two passing groups/four commands')
 if not c['passed'] or len(c['groups'])!=3 or len(c['commands'])!=6 or any(x['product_returncode']!=0 for x in c['commands']):raise ValueError('Focused caller proof changed')
 if not n['passed'] or set(n['targets'])!={'PTPcmTest'} or len(n['commands'])!=16 or any(c['returncode'] for c in n['commands']):raise ValueError('Native cross-build proof changed')
 if old['run_rc']!=20 or old['expected_rc']!=20 or not old['passed'] or new['run_rc']!=0 or new['expected_rc']!=0 or not new['passed']:raise ValueError('Expected failing baseline / passing candidate changed')
 if (ROOT/'baseline-probe/probe.c').read_bytes()!=(ROOT/'candidate-probe/probe.c').read_bytes() or (ROOT/'output-probe.c').read_bytes()!=(ROOT/'candidate-probe/probe.c').read_bytes():raise ValueError('Not identical legal probe source')
 for key in ('source_files_before','source_files_after'):
  if any(d[key]!=7633 for d in (h,c,n,old,new)):raise ValueError('Full source before/after counts differ')
 for d,src in ((h,ROOT/'source-final'),(c,ROOT/'source-final'),(n,ROOT/'source-final'),(old,ROOT/'baseline'),(new,ROOT/'source-final')):
  check_recorded_files(d,src);pinned_map(d.get('external_sdk_dependencies',{}))
 for proof in (h,c):
  for name in ('sanitizer_runtime_inputs','pinned_sdk_inputs'):pinned_map(proof[name])
 for d in (old,new):
  pinned_map(d['runtime_inputs']);pinned_map(d['sdk_inputs'])
 require(Path(n['compiler']),n['compiler_sha256'],allow_symlink=True)
 for name,expected in n['runtime_inputs'].items():require(Path(n['runtime_paths'][name]),expected,allow_symlink=True)
 t=n['targets']['PTPcmTest'];pinned_map(t['external_sdk_dependencies'])
 if t['binary_sha256']!='48acf5e0b61042dd7774c87253a016d09d005f9663880552d5766ce2a35f494f' or t['binary_bytes']!=45988:raise ValueError('Unexpected native artifact')
 require(ROOT/'native-final/PTPcmTest',t['binary_sha256'],t['binary_bytes'])
 if not mock['passed'] or mock['tests']!=17 or mock['errors'] or mock['failures'] or prepared['status']!='PREPARED_NOT_RUN':raise ValueError('Mock/preparation scope mismatch')
 check_recorded_files(mock,ROOT/'source-final')
 check_recorded_files(prepared,ROOT/'source-final')
 closure=EXTRA_SOURCE|OVERLAYS|set(h['canonical_dependencies'])|set(c['canonical_dependencies'])|set(old['canonical_dependencies'])|set(new['canonical_dependencies'])|set(t['dependencies'])|set(n['python_declaration_inputs'])
 for p in closure:
  if p not in s['source'] or p not in b['source']:raise ValueError('Missing required execution recipe/module/source bytes: '+p)
 for d,expected in ((h['canonical_dependencies'],s['source']),(c['canonical_dependencies'],s['source']),(new['canonical_dependencies'],s['source']),(old['canonical_dependencies'],b['source']),(t['dependencies'],s['source'])):
  if any(expected.get(p)!=v for p,v in d.items()):raise ValueError('Execution-source hash mismatch')
 index={}
 def add(destination,path):
  parts=PurePosixPath(destination)
  if parts.is_absolute() or '..' in parts.parts or not destination or '\\' in destination:raise ValueError('Unsafe archive path')
  if destination in index:raise ValueError('Duplicate archive member: '+destination)
  index[destination]=record(path)
 for folder in ('host-final','host-callers-final','caller-tools','native-final','baseline-probe','candidate-probe','tools','tools-native','overlay-final','qualifier'):
  for p in sorted((ROOT/folder).rglob('*')):
   if p.is_symlink():raise ValueError('Proof symlink refused')
   if p.is_file():add('proof/'+p.relative_to(ROOT).as_posix(),p)
 for name in ('baseline-manifest.json','source-manifest.json','final.patch','output-probe.c','handoff.json','handoff-complete.json','handoff-complete-v2.json','handoff-final.json'):add('proof/'+name,ROOT/name)
 for label in ('baseline','source-final'):
  for p in sorted(closure):add('execution-source/'+label+'/'+p,ROOT/label/p)
 add('packaging/package_pcm_frame_output_safety.py',Path(__file__).resolve())
 add('packaging/failed-v1.py',Path('/private/tmp/package_pcm_frame_output_safety.py'))
 add('packaging/failed-v1.log',Path('/private/tmp/protracker-pcm-frame-output-safety-package.log'))
 add('packaging/failed-v1-description.json',Path('/private/tmp/protracker-pcm-frame-output-safety-packaging-failure.json'))
 add('packaging/pre-caller-unexecuted-draft.py',Path('/private/tmp/package_pcm_frame_output_safety.pre-caller-draft.py'))
 return {'members':index,'source_closure':sorted(closure),'baseline':b,'final':s,'host':h,'callers':c,'native':n,'baseline_probe':old,'candidate_probe':new,'mock':mock,'prepared':prepared}

def archive_bytes(index):
 buf=io.BytesIO()
 with gzip.GzipFile(fileobj=buf,mode='wb',filename='',mtime=0,compresslevel=9) as compressed:
  with tarfile.open(fileobj=compressed,mode='w',format=tarfile.PAX_FORMAT) as tar:
   for name,r in sorted(index.items()):
    path=Path(r['path']);require(path,r['sha256'],r['bytes']);data=path.read_bytes()
    info=tarfile.TarInfo(name);info.size=r['bytes'];info.mode=r['mode'];info.uid=info.gid=0;info.mtime=0;info.uname=info.gname='';info.pax_headers={}
    tar.addfile(info,io.BytesIO(data))
 return buf.getvalue()

def verify_archive(path,index):
 seen=set();total=0
 with tarfile.open(path,'r:gz') as tar:
  for item in tar:
   if not item.isfile() or item.name in seen or item.name not in index or item.uid or item.gid or item.mtime or item.uname or item.gname:raise ValueError('Unsafe/unexpected tar member')
   r=index[item.name];data=tar.extractfile(item).read()
   if item.size!=r['bytes'] or item.mode!=r['mode'] or sha(data)!=r['sha256'] or data!=Path(r['path']).read_bytes():raise ValueError('Tar member/origin byte mismatch')
   seen.add(item.name);total+=len(data)
 if seen!=set(index):raise ValueError('Missing archive members')
 return len(seen),total

def main():
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--out',type=Path,required=True);a.add_argument('--qa',type=Path,required=True);args=a.parse_args()
 out=args.out.resolve();qa=args.qa.resolve()
 if out.exists() or qa.exists():raise ValueError('Fresh output and external QA required')
 for p in (ROOT,Path(__file__).resolve()):
  if p==out or p in out.parents or out in p.parents or p==qa or p in qa.parents or qa in p.parents:raise ValueError('Origin/output ancestry collision')
 if out==qa or out in qa.parents or qa in out.parents:raise ValueError('External QA must be separate from output')
 d=prepare();index=d['members'];payload=archive_bytes(index);repeat=archive_bytes(index)
 if payload!=repeat:raise ValueError('Archive was not deterministic')
 out.mkdir();copies=[]
 def copy(name,origin):
  path=out/name
  if path.exists():raise ValueError('Duplicate control destination')
  data=origin.read_bytes();path.write_bytes(data)
  if path.read_bytes()!=data:raise ValueError('Control copy changed')
  copies.append({'destination':name,'kind':'exact-copy','origin':record(origin),'bytes':len(data),'sha256':sha(data)})
 def compress(name,origin):
  path=out/name;data=origin.read_bytes();encoded=gzip.compress(data,compresslevel=9,mtime=0);path.write_bytes(encoded)
  if gzip.decompress(path.read_bytes())!=data:raise ValueError('Lossless gzip origin changed')
  copies.append({'destination':name,'kind':'lossless-gzip-derivation','origin':record(origin),'bytes':len(encoded),'sha256':sha(encoded)})
 (out/'payload.tar.gz').write_bytes(payload)
 count,original_bytes=verify_archive(out/'payload.tar.gz',index)
 member_index={'scope':'Every indexed member is an exact original byte copy; normalized tar owner/time, original mode. Full7633 inventories preserved; selected actual execution-source closure only, not complete source tree/toolchain binary redistribution.',
  'base_commit':d['baseline']['base_commit'],'base_tree':d['baseline']['base_tree'],'source_files_per_inventory':7633,'overlays':d['final']['overlays'],'dirty_exclusions':d['final']['dirty_exclusions'],
  'baseline_archive_fingerprint_only':d['baseline']['archive'],'source_closure':d['source_closure'],'member_count':count,'original_bytes':original_bytes,'members':index,
  'payload':{'bytes':len(payload),'sha256':sha(payload)},'claims':{'host':'PASS initial PCM/filter two groups/four commands plus focused caller three groups/six commands','same_probe':'expected baselineRC20 -> candidateRC0, four legal cases','native':'cross-build PASS only; portable fixture NOT RUN','qualifier':'PREPARED_NOT_RUN; seventeen simulated guards only','no_acceptance':['native execution','Exec Fast allocator','device/MMIO','physical','audio/listening','timing/scheduling']}}
 index_bytes=json_bytes(member_index);(out/'member-index.json.gz').write_bytes(gzip.compress(index_bytes,compresslevel=9,mtime=0))
 if gzip.decompress((out/'member-index.json.gz').read_bytes())!=index_bytes:raise ValueError('Index gzip mismatch')
 copies.append({'destination':'member-index.json.gz','kind':'authored-lossless-gzip-index','bytes':(out/'member-index.json.gz').stat().st_size,'sha256':sha((out/'member-index.json.gz').read_bytes()),'uncompressed_bytes':len(index_bytes),'uncompressed_sha256':sha(index_bytes)})
 copies.append({'destination':'payload.tar.gz','kind':'deterministic-tar-gzip-derivation','member_count':count,'original_bytes':original_bytes,'bytes':len(payload),'sha256':sha(payload)})
 for name in ('baseline-manifest.json','source-manifest.json'):compress(name+'.gz',ROOT/name)
 for dst,src in (('host-checks.json.gz','host-final/host-checks.json'),('host-callers.json.gz','host-callers-final/host-checks.json'),('native-manifest.json.gz','native-final/manifest.json'),('baseline-probe.json','baseline-probe/result.json'),('candidate-probe.json','candidate-probe/result.json'),('handoff.json','handoff-final.json'),('qualifier-prepared.json','qualifier/prepared.json'),('mock-proof.json','qualifier/mock-proof.json')):
  if dst.endswith('.gz'):compress(dst,ROOT/src)
  else:copy(dst,ROOT/src)
 copy('packager.py',Path(__file__).resolve())
 readme=f'''The portable PCM resampled-frame output fix protects source descriptor and full declared master capacity before publishing the frame count. This package retains exact source/host/cross-build/probe/preparation evidence; it contains no native execution or live coordination result.

Both original complete7,633-file manifests are losslessly preserved with all Git metadata, three explicit PCM overlays and16 dirty-path exclusions. The baseline84,480,000-byte export tar is fingerprinted rather than duplicated. The archive contains {count} exact original regular members ({original_bytes:,} uncompressed bytes), including selected baseline/final execution sources, Python modules/generator/canonical recipe, shared runner recipe, every command/log/product/preprocessed assertion record, helpers, patch, all three overlay bytes and original handoff history. Toolchain/SDK external paths and hashes are retained and rechecked; their external binary trees are not redistributed. No generated fixture is invented where the host report records none.

Host evidence: original PCM/filter two ASan/UBSan groups and four commands passed; separate existing sampler/Paula preview and generated empty-caller groups passed six commands with exact full-M provenance and actual generated fixture archives; original four-case valid aligned probe failed as expectedRC20, while the identical candidate probe returnedRC0 without master corruption. Native evidence is cross-build PASS only: PTPcmTest45,988bytes SHA48acf5e0b61042dd7774c87253a016d09d005f9663880552d5766ce2a35f494f, five ordered translation units, canonical and completeSDK dependency closure, original portable SDKassert, seven runtime pins. No native fixture was run.

Qualifier evidence is PREPARED_NOT_RUN. Seventeen host-only mocked guards passed using fake process/profile/Guest/IPC/time/lock/runner state in private temporary folders. Both PCM FRAME OUTPUT PASS and existing PCM/WAV PASS markers are required later, using unchanged --wav-output (wav-core label),90s native/140s subprocess/250s overall elapsed gate, fresh explicit identity and11 independent absences over more than10s. The live caller must also impose an external250s watchdog and stop/HOLD on uncertain timeout. No original process was discovered, target reserved, real shared lock acquired, Guest contacted or release acknowledgment supplied by this work. --repo must be explicit from the private helper location.

The original handoff.json and handoff-complete.json remain exact archive members. The complete record retained stale host-only top wording when cross-build fields were appended; copied handoff.json here is the explicitly derived final record extending the preserved v2 scope correction with separately requested caller proof and original fingerprints, and no proof record was rewritten.

The first packaging helper stopped before creating its proposed output: it counted dirty-exclusion metadata keys rather than the16 paths. Its exact helper, failure log and packaging-only description are archived. This v2 correction changes the private packaging shape assertion and preserves all product proof bytes; no test/build/native/target retry is implied.

Host/build/mock/probe success does not establish Exec Fast-pool/TypeOfMem, device/card/MMIO, physical, audio/listening or scheduling/timing acceptance. Native qualification remains pending a separately coordinated root-owned window.
'''
 (out/'README.md').write_text(readme)
 copies.append({'destination':'README.md','kind':'authored-scope-note','bytes':(out/'README.md').stat().st_size,'sha256':sha((out/'README.md').read_bytes())})
 provenance={'scope':'Private compact evidence packaging only; no tests/builds/native execution/source/repository/target operation.', 'helper':record(Path(__file__).resolve()),'records':copies,'source_inventory_verification':'Both immutable inventories matched before and after packaging, every source hash retained without field substitution','member_index_sha256':sha((out/'member-index.json.gz').read_bytes())}
 (out/'provenance.json').write_bytes(json_bytes(provenance))
 files=sorted(p for p in out.iterdir() if p.is_file());sums=''.join(record(p)['sha256']+'  '+p.name+'\n' for p in files);(out/'SHA256SUMS').write_text(sums)
 if snapshot(ROOT/'baseline')!=d['baseline']['source'] or snapshot(ROOT/'source-final')!=d['final']['source']:raise ValueError('Immutable source changed during packaging')
 for r in index.values():require(Path(r['path']),r['sha256'],r['bytes'])
 for r in copies:
  require(out/r['destination'],r['sha256'],r['bytes'])
  if 'origin' in r:
   require(Path(r['origin']['path']),r['origin']['sha256'],r['origin']['bytes'])
   actual=(out/r['destination']).read_bytes()
   if r['kind']=='lossless-gzip-derivation':actual=gzip.decompress(actual)
   if actual!=Path(r['origin']['path']).read_bytes():raise ValueError('Control origin mismatch')
 parsed={}
 for line in (out/'SHA256SUMS').read_text().splitlines():
  expected,name=line.split('  ',1)
  if name in parsed or name=='SHA256SUMS':raise ValueError('Duplicate/self checksum')
  parsed[name]=expected;require(out/name,expected)
 if set(parsed)!={p.name for p in out.iterdir() if p.is_file() and p.name!='SHA256SUMS'}:raise ValueError('Checksum coverage mismatch')
 final_count,final_bytes=verify_archive(out/'payload.tar.gz',index)
 report={'scope':'External packaging QA, not test/build/target acceptance','passed':True,'package':str(out),'files':len(list(out.iterdir())),'bytes':sum(p.stat().st_size for p in out.iterdir()),'checksum_rows':len(parsed),'payload':record(out/'payload.tar.gz'),'member_index':record(out/'member-index.json.gz'),'provenance':record(out/'provenance.json'),'sums':record(out/'SHA256SUMS'),'archive_members':final_count,'archive_original_bytes':final_bytes,'source_inventory_files_each':7633,'copied_source_files_each':len(d['source_closure']),'overlays':3,'dirty_exclusions_preserved':16,'native_status':'NOT_RUN','qualifier_status':'PREPARED_NOT_RUN','origin_and_full_source_reverified':True,'deterministic_repeat':True}
 qa.write_bytes(json_bytes(report));print(json.dumps(report,indent=2))
if __name__=='__main__':main()
