from pathlib import Path
import datetime,gzip,hashlib,json,shutil,subprocess,time
repo=Path('/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A')
private=Path('/private/tmp/protracker-capture-current-_eke0mzq')
run=repo/'build/dev/capture-current-qualification-1790988615201455000'
out=repo/'evidence/enhanced-editor/capture-current-native'
assert not out.exists(), 'Refuse existing destination'
sha=lambda b:hashlib.sha256(b).hexdigest()
source_path=private/'source-manifest.json'; source=json.loads(source_path.read_text())
native_path=private/'native/manifest.json'; native=json.loads(native_path.read_text())
host=json.loads((private/'host-checks.json').read_text())
qualification=json.loads((run/'qualification-result.json').read_text())
assert source['base_commit']=='0adacb2cef6e657931d78e4c8dc82d0ad8279601'
assert len(source['source_inputs'])==7443 and len({x['path'] for x in source['source_inputs']})==7443
assert source['overlay_paths']==[] and not source['overlays']
assert len(source['excluded_dirty_source_checks'])==9
assert all(x['matches_commit'] and not x['matches_dirty_checkout'] for x in source['excluded_dirty_source_checks'])
assert host['passed'] and host['tests_run']==2 and host['failures']==host['errors']==0
assert len(host['canonical_dependencies'])==64 and len(host['python_imports'])==4
assert host['frozen_input_checks_before_after'] and native['frozen_inputs_verified_before_after']
assert qualification['passed'] and len(qualification['cases'])==2
assert qualification['source_tree']==source['base_tree']==native['base_tree']
assert qualification['verified_manifest']==native
assert [row[0] for row in qualification['original_processes']]==[19081]
for case in qualification['cases']:
 label=case['candidate']['label']; target=native['targets'][case['candidate']['binary']]
 q=Path(case['run']); record=json.loads((q/'result.json').read_text()); independent=json.loads((q/'independent-cleanup.json').read_text())
 assert record==case['result'] and independent==case['independent_cleanup']
 assert record['passed'] and record['run_files_cleaned'] and record[label+'_returncode']=='0'
 assert record[case['candidate']['binary']+'_sha256']==target['binary_sha256']==case['candidate']['sha256']
 assert len(independent['observations'])==11 and independent['passed']
 assert all(not any(x.values()) for x in independent['observations'])
 assert independent['processes']==qualification['original_processes']
 assert (q/(label+'.log')).read_text()==case['native_log']
 assert 'EXEC MEMORY PASS: '+('143' if label=='capture' else '70')+' Fast allocations, zero owned bytes' in case['native_log']
 assert all('ch%d_dma=0'%i in independent['final_idle']['audio'].split('\t') for i in range(4))
 assert 'Paused=false' in independent['final_idle']['status'].split('\t')
 if label=='editor-capture':assert all(x in case['native_log'] for x in ('CAPTURE SESSION PASS:','AMIGUS CAPTURE PASS:','EDITOR CAPTURE PASS:'))
 else:assert 'CAPTURE STAGING PASS:' in case['native_log']
out.mkdir(parents=True)
records=[]
def put(relative,data,kind,origins=None,derivation=None):
 dest=out/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
 assert dest.read_bytes()==data
 item={'path':relative,'bytes':len(data),'sha256':sha(data),'kind':kind}
 if origins:item['origins']=[{'path':str(p),'bytes':p.stat().st_size,'sha256':sha(p.read_bytes())} for p in origins]
 if derivation:item['derivation']=derivation
 if kind=='byte-copy':assert all(dest.read_bytes()==p.read_bytes() for p in origins)
 records.append(item)
def cp(src,dest):put(dest,src.read_bytes(),'byte-copy',[src])
def derived(relative,value,origins,explanation):put(relative,(json.dumps(value,indent=2)+'\n').encode(),'derived',origins,explanation)
for name in ('host-checks.json','host-checks.log','validate_host.py','suite-1.d','suite-2.d'):
 cp(private/name,'host/'+name)
cp(private/'native-build.log','native/build.log')
cp(private/'native/compile-commands.json','native/compile-commands.json')
cp(Path('/private/tmp/protracker_capture_native_build.py'),'native/build-helper.py')
for name in ('PTExecCaptureTest','PTExecEditorCaptureTest'):
 cp(private/'native'/(name+'.build.log'),'native/'+name+'.build.log')
 cp(private/'native'/(name+'.d'),'native/'+name+'.d')
# Preserve all original frozen-source records once, without copying the source checkout.
compact=dict(source)
compact['original_manifest']={'path':str(source_path),'bytes':source_path.stat().st_size,'sha256':sha(source_path.read_bytes())}
compact['derivation']='Compact serialization of all original manifest fields and every one of 7443 source_inputs; deterministic gzip mtime0. No source records removed or rewritten.'
raw=json.dumps(compact,separators=(',',':')).encode()+b'\n'
packed=gzip.compress(raw,mtime=0)
assert json.loads(gzip.decompress(packed))['source_inputs']==source['source_inputs']
put('frozen-source-manifest.json.gz',packed,'derived',[source_path],'Deterministic gzip of compact JSON retaining all original source fields,7443 source records, base/tree, zero overlays and 9 dirty exclusions, plus original manifest byte fingerprint.')
scoped={k:v for k,v in native.items() if k!='export_file_hashes'}
scoped['original_manifest']={'path':str(native_path),'bytes':native_path.stat().st_size,'sha256':sha(native_path.read_bytes())}
scoped['export_file_hashes_snapshot']='../frozen-source-manifest.json.gz:source_inputs path/sha256 for all 7443 original entries'
scoped['scope_note']='Derived scoped manifest; compiler/runtime, ordered TUs,39/62 dependency hashes, binaries and compile-command provenance unchanged. Full source hash map stored once in frozen snapshot.'
assert {x['path']:x['sha256'] for x in source['source_inputs']}==native['export_file_hashes']
derived('native/scoped-manifest.json',scoped,[native_path,source_path],'Remove only duplicated export_file_hashes; preserve original fingerprint and full frozen snapshot reference.')
cp(run/'qualification-result.json','qualification-result.json')
for label in ('capture','editor-capture'):
 cp(run/(label+'-runner.log'),'runs/'+label+'/runner.log')
 case=next(x for x in qualification['cases'] if x['candidate']['label']==label)
 q=Path(case['run'])
 for f in sorted(q.iterdir()):
  assert f.is_file(), 'Unexpected nested record directory'
  cp(f,'runs/'+label+'/'+f.name)
runner_origins=[run/('runner-'+label)/'tools/shared_infra_render_files.py' for label in ('capture','editor-capture')]
assert runner_origins[0].read_bytes()==runner_origins[1].read_bytes()
put('runner/shared_infra_render_files.py',runner_origins[0].read_bytes(),'byte-copy',runner_origins)
cp(repo/'build/dev/qualify_capture_current.py','runner/qualify_capture_current.py')
cp(Path(__file__),'packaging-helper.py')
# Historical records remain outside this package; only references and original hashes are added.
prior=[]
for directory in ('capture-format','capture-transfer'):
 base=repo/'evidence/enhanced-editor'/directory
 for f in sorted(base.iterdir()):
  if f.is_file():prior.append({'path':f.relative_to(repo).as_posix(),'bytes':f.stat().st_size,'sha256':sha(f.read_bytes())})
derived('historical-prelaunch-reference.json',{'status':'Preserved original 30 September prelaunch NOT RUN; no files modified or substituted. This later qualification has its own exact candidates and runs.','records':prior},[repo/'evidence/enhanced-editor/capture-format/README.md',repo/'evidence/enhanced-editor/capture-transfer/README.md'],'Read-only fingerprints of original format/transfer history; current native qualification does not rewrite historical NOT RUN.')
coordination={'source':'Root task reports supplied directly to packaging subagent; no independent live coordination verification by packager.','release':{'sent_after_all_pass':True,'acknowledged':True,'amiconnect_revision':29,'no_hold':True,'no_reservation':True},'executed_root_script':{'path':str(repo/'build/dev/qualify_capture_current.py'),'root_reports_current_bytes_are_exact_historical_execution':True,'no_embedded_execution_hash_claimed':True,'archived_copy':'runner/qualify_capture_current.py','copy_sha256':sha((repo/'build/dev/qualify_capture_current.py').read_bytes())},'target':{'root_reported_cpu':'68030','recorded_original_process_pid':19081,'recorded_profile':'A1200-030-DEV.uae','root_reported_localhost':True},'scope':'No real input, card/input MMIO, physical operation, audio or human listening acceptance; packaging performs no target operation.'}
derived('root-reported-coordination.json',coordination,[repo/'build/dev/qualify_capture_current.py'],'Manual attribution to direct root reports including release ACK revision 29 and exact historical script confirmation; source script copy bytes independently verified, coordination not independently rechecked.')
readme='''# Current recording native qualification

The two current injected recording fixtures passed host sanitizers, cross-build and separate shared030 native runs. Product source is committed `0adacb2`; its zero-overlay export contains 7443 committed inputs. The 64 host dependencies,39/62 native dependencies and nine excluded dirty paths are recorded. Unrelated editor/display work was excluded.

| Fixture | Native run | Result | Owned Fast allocations |
|---|---|---|---:|
| `PTExecCaptureTest` | `1790988616063281000` | RC0; CAPTURE STAGING PASS | 143; zero owned bytes |
| `PTExecEditorCaptureTest` | `1790988631933347000` | RC0; CAPTURE SESSION, AMIGUS CAPTURE and EDITOR CAPTURE PASS | 70; zero owned bytes |

The two ASan/UBSan host groups passed in 9.358 seconds (9.975 seconds including provenance work). The native candidates are 114852 bytes/SHA `ab5ceb7e8f4773b3a41ad741845ca83c4e86bf326432a514b8a0f00c9ecb85f9` and 166164 bytes/SHA `2141a7734809f747047d0af7a06f29df97f69115539b052161e6a2fe5b5b31c7`. Ordered commands, pinned compiler, seven runtime fingerprints, included-fixture dependencies and before/after frozen-input checks are retained.

Each run used the unchanged 90-second fixture bound with a separate 140-second process guard. The root qualification used its 350-second overall guard. Each nested result records exact cleanup; each separate independent cleanup records 11 observations across 10 seconds with both owned paths absent, the original sole PID 19081/profile, and a final running guest with all four Paula DMA channels off. The top `qualification-result.json` is preserved separately from `runs/*/result.json` and independent cleanup records.

Root reported that the shared window was explicitly released after all checks and acknowledged by AmiConnect revision 29 with no hold or reservation. This coordination report is attributed in `root-reported-coordination.json`; the packaging subagent did not perform a live ownership check. Root also confirmed the archived root script has not changed since execution. Its archived bytes are independently verified; no embedded execution-time script hash is claimed.

`frozen-source-manifest.json.gz` retains all original 7443 path/blob/mode/size/SHA records, base commit/tree, zero overlays and nine dirty exclusions. `native/scoped-manifest.json` removes only the duplicated full source hash map, points to that snapshot, and preserves the original manifest fingerprint. Every raw copied file has its origin, byte count and SHA in the unique provenance manifest. Derived records are labelled separately. The scripts are historical evidence; this directory is not an instruction to rerun them.

The 30 September format/transfer prelaunch remains **NOT RUN** and unchanged. `historical-prelaunch-reference.json` records read-only fingerprints of that history. These later current-source runs are separate evidence.

This qualifies synthetic/injected format, staging, transfer, session/editor ownership and Exec Fast allocation cleanup. It does not qualify actual input or card MMIO, physical capture, timing, audible playback or human listening. No product source, tests, builds, targets, locks or staging were changed or rerun during packaging.
'''
put('README.md',readme.encode(),'authored',derivation='Evidence summary from preserved artifact records and explicitly attributed root reports; no added acceptance tier.')
for item in records:
 f=out/item['path'];assert f.stat().st_size==item['bytes'] and sha(f.read_bytes())==item['sha256']
 if item['kind']=='byte-copy':assert all(f.read_bytes()==Path(o['path']).read_bytes() for o in item['origins'])
for item in prior:
 f=repo/item['path'];assert f.stat().st_size==item['bytes'] and sha(f.read_bytes())==item['sha256']
provenance={'id':'capture-current-native-'+str(time.time_ns()),'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'package_scope':'Host evidence and completed shared030 injected-fixture records; packaging only, no product edits/reruns/targets/staging.','product_commit':source['base_commit'],'product_tree':source['base_tree'],'packaging_head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),'original_private_package':str(private),'original_root_qualification':str(run),'frozen_source_inputs':7443,'native_dependency_counts':{'PTExecCaptureTest':39,'PTExecEditorCaptureTest':62},'host_dependencies':64,'zero_overlays':True,'excluded_dirty_paths':9,'historical_files_untouched_verified':True,'byte_copies_verified_against_origins':True,'files':records}
name='provenance-'+provenance['id'].split('-')[-1]+'.json'
data=(json.dumps(provenance,indent=2)+'\n').encode();(out/name).write_bytes(data)
sums=''.join(x['sha256']+'  '+x['path']+'\n' for x in sorted(records,key=lambda x:x['path']))+sha(data)+'  '+name+'\n'
(out/'SHA256SUMS').write_text(sums)
assert sha((out/name).read_bytes())==sha(data)
print(json.dumps({'directory':str(out),'provenance':name,'provenance_sha256':sha(data),'proof_files':len(records),'total_files':len(records)+2,'byte_copies':sum(x['kind']=='byte-copy' for x in records),'derived_records':sum(x['kind']=='derived' for x in records),'total_bytes':sum(f.stat().st_size for f in out.rglob('*') if f.is_file()),'frozen_source_inputs':7443,'copied_bytes_verified':True,'history_untouched':True},indent=2))
