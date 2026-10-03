"""Host-only audit of saved records; no helper imports, targets, process or lock queries."""
import ast, hashlib, json, math, pathlib, re, shlex, subprocess, tempfile, time
P=pathlib.Path
repo=P('/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A')
root=P('/private/tmp/protracker-mod-current-root-1cn1g3y0')
build=P('/private/tmp/protracker-mod-after-pcm-build-eqyyzl74')
qual=repo/'build/dev/mod-output-safety-current-qualification-1791046730931142000'
sha=lambda b:hashlib.sha256(b).hexdigest()
load=lambda p:json.loads(P(p).read_bytes())
records={}
def check_file(path,expected,bytes_=None):
 path=P(path); data=path.read_bytes();assert sha(data)==expected,(str(path),'hash');assert bytes_ is None or len(data)==bytes_,(str(path),'bytes');records[str(path)]={'sha256':sha(data),'bytes':len(data)};return data
def direct_record(path):
 path=P(path);data=path.read_bytes();records[str(path)]={'sha256':sha(data),'bytes':len(data)};return json.loads(data)
def commit_bytes(tree,path):return subprocess.check_output(['git','show',tree+':'+path],cwd=repo,timeout=10)
q=direct_record(qual/'qualification-result.json');assert records[str(qual/'qualification-result.json')]['sha256']=='9c82b75e031085aec5d8b238b98c128442a79c40cae165c6fc6e8f719d7966c9'
assert q['passed'] is True and len(q['cases'])==1
manifest=direct_record(build/'native-final/manifest.json');assert sha((build/'native-final/manifest.json').read_bytes())=='4027777578e796af0eaa3e061559c98a022df2803ee90b170291ad3e58af36dc';assert manifest==q['verified_manifest']
current_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True,timeout=10).strip();current_tree=subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=repo,text=True,timeout=10).strip();assert current_tree==q['source_tree']=='8869e6e261efe2905c820e5fecf228df323df461'
r=manifest['targets']['PTExecModStreamTest'];assert len(r['dependencies'])==23 and len(r['external_sdk_dependencies'])==74
for path,h in r['dependencies'].items():
 check_file(build/'source'/path,h);check_file(repo/path,h);assert sha(commit_bytes(current_tree,path))==h,path
assert not any('playback_pcm' in p for p in r['dependencies'])
for path,h in r['external_sdk_dependencies'].items():check_file(path,h)
check_file(manifest['compiler'],manifest['compiler_sha256'])
for name,h in manifest['runtime_inputs'].items():check_file(manifest['runtime_paths'][name],h)
assert len(manifest['runtime_inputs'])==7
check_file(manifest['python']['path'],manifest['python']['sha256'])
for item in manifest['helper_inputs']:check_file(item['path'],item['sha256'],item['bytes'])
for cmd in manifest['commands']:
 assert cmd['returncode']==0,cmd['label'];item=cmd['log'];check_file(item['path'],item['sha256'],item['bytes'])
assert len(manifest['commands'])==20
for item in [manifest['source_manifest'],manifest['pins'],manifest['canonical_recipe'],r['preprocessed_wrapper'],r['build_log']]:check_file(item['path'],item['sha256'],item['bytes'])
source=load(manifest['source_manifest']['path']);assert len(source['source'])==7633
for path,h in source['source'].items():assert sha((build/'source'/path).read_bytes())==h,path
for path,h in manifest['python_declaration_inputs'].items():check_file(build/'source'/path,h);assert sha(commit_bytes(current_tree,path))==h
recipe=manifest['canonical_recipe'];assert sha(commit_bytes(current_tree,'tools/build_core_tests.py'))==recipe['sha256']
flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core','-Ibuild/dev','-fbbb=-']
units=['tests/native_exec_mod_stream_test.c','src/platform/mod_file.c','src/platform/file_save.c','src/core/safe_save.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
assert r['source_inputs']==units and r['flags']==manifest['flags']==flags
commands_data=check_file(manifest['compile_commands'],manifest['compile_commands_sha256'],manifest['compile_commands_bytes']);compile_commands=json.loads(commands_data)['commands'];assert len(compile_commands)==1
print('COMPILE_COMMAND_KEYS',list(compile_commands[0]))
argv=compile_commands[0]['argv'];assert argv[0]==manifest['compiler'] and argv[1:1+len(flags)]==flags and argv[1+len(flags):1+len(flags)+len(units)]==units
# Reconstruct canonical recipe order and SDK assertion from text/AST only.
tree=ast.parse(P(recipe['path']).read_text());ordinary=None;exec_node=None
for n in ast.walk(tree):
 if isinstance(n,ast.Assign):
  for target in n.targets:
   if isinstance(target,ast.Subscript) and isinstance(target.value,ast.Name) and target.value.id=='inputs' and isinstance(target.slice,ast.Constant):
    if target.slice.value=='PTModStreamTest':ordinary=ast.literal_eval(n.value)
    if target.slice.value=='PTExecModStreamTest':exec_node=n.value
assert ordinary[1:]==units[1:] and isinstance(exec_node,ast.List) and len(exec_node.elts)==2
assert ast.literal_eval(exec_node.elts[0])==units[0] and ast.unparse(exec_node.elts[1])=="*inputs['PTModStreamTest'][1:]"
macro='#define assert(__e) ((__e) ? (void)0 : __assert_func (__FILE__, __LINE__, __ASSERT_FUNC, #__e))'
pp=P(r['preprocessed_wrapper']['path']).read_text();assert [x for x in pp.splitlines() if x.startswith('#define assert(')][-1]==r['last_assert_macro']==macro
# Compile full-M records preserve exact order and closure, without invoking compiler.
canon=set();sdk=set()
for i,unit in enumerate(units):
 cmd=next(c for c in manifest['commands'] if c['label']==f'PTExecModStreamTest-dependencies-{i:02d}');assert cmd['argv']==[manifest['compiler'],*flags,'-M',unit]
 raw=P(cmd['log']['path']).read_text().replace('\\\n',' ');deps=shlex.split(raw.split(':',1)[1]);
 for dep in deps:
  p=P(dep)
  if p.is_absolute():sdk.add(str(p.resolve()))
  else:canon.add(str((build/'source'/p).resolve().relative_to((build/'source').resolve())))
assert canon==set(r['dependencies']) and sdk==set(r['external_sdk_dependencies'])
case=q['cases'][0];assert case['candidate']['bytes']==70824 and case['candidate']['sha256']==r['binary_sha256']
binary=check_file(build/'native-final/PTExecModStreamTest',r['binary_sha256'],70824);assert binary[:4]==bytes.fromhex('000003f3')
runner=qual/'runner-mod-stream';check_file(runner/'build/dev/PTExecModStreamTest',r['binary_sha256'],70824)
runner_h='df9c7f37252bb27e9315aa244854db12a3186ba987924c0d094f5142ae968978';check_file(runner/'tools/shared_infra_render_files.py',runner_h);assert sha(commit_bytes(current_tree,'tools/shared_infra_render_files.py'))==runner_h
native=direct_record(P(case['run'])/'result.json');independent=direct_record(P(case['run'])/'independent-cleanup.json');assert native==case['result'] and independent==case['independent_cleanup']
assert native['passed'] is True and native['mod-stream_returncode']=='0' and native['sample_staging_clean'] is True and native['run_files_cleaned'] is True and native['PTExecModStreamTest_sha256']==r['binary_sha256']
log=(P(case['run'])/'mod-stream.log').read_text();records[str(P(case['run'])/'mod-stream.log')]={'bytes':len(log.encode()),'sha256':sha(log.encode())};assert log==case['native_log']
assert len(log.splitlines())==4 and 'MOD OUTPUT PRESERVATION PASS:' in log and 'MOD STREAM PASS:' in log and 'workspace=7164' in log and 'EXEC MEMORY PASS: 13 Fast allocations, zero owned bytes' in log
identity=direct_record(root/'fresh-identity-next.json');assert q['identity_record']==identity and q['identity_sha256']==records[str(root/'fresh-identity-next.json')]['sha256']
assert q['original_processes']==[identity['process']] and independent['processes']==[identity['process']]
check_file(identity['profile'],identity['profile_sha256'])
def idle(d):
 assert [x for x in d['status'].split('\t') if x.startswith('Paused=')]==['Paused=false']
 assert 'model=68030' in d['cpu'].split('\t')
 assert all('ch%d_dma=0'%i in d['audio'].split('\t') for i in range(4))
idle(q['initial_idle']);idle(independent);idle(independent['final_idle']);assert all('ch%d_dma=0'%i in native['cleanup_audio'].split('\t') for i in range(4))
expected={str(P('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra/runtime/Dev/Tests')/P(case['run']).name),str(P('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra/runtime/Dev/Tests')/('launch-'+P(case['run']).name))}
def absent(rows,times):
 assert len(rows)==len(times)==11
 assert all(set(row)==expected and all(v is False for v in row.values()) for row in rows)
 assert all(isinstance(v,(int,float)) and not isinstance(v,bool) and math.isfinite(v) and v>=0 for v in times)
 assert all(a<b for a,b in zip(times,times[1:])) and times[-1]-times[0]>10
 return times[-1]-times[0]
internal_span=absent(independent['observations'],independent['observation_elapsed_s']);assert independent['passed'] is True and independent['observed_elapsed_s']>=independent['observation_elapsed_s'][-1]
separate=direct_record(root/'separate-idle-result.json');separate_span=absent([x['paths'] for x in separate['observations']],[x['elapsed_s'] for x in separate['observations']]);assert separate['passed'] is True and separate['observation_span_s']==separate_span and separate['initial_processes']==separate['final_processes']==[identity['process']];idle(separate['initial_idle']);idle(separate['final_idle'])
outer=direct_record(root/'native-execution-next.json');outer_return=direct_record(root/'separate-idle-execution.json');assert outer['passed'] is True and outer['returncode']==0 and outer['elapsed_s']<outer['outer_bound_s']==250;assert outer_return['passed'] is True and outer_return['returncode']==0 and outer_return['elapsed_s']<outer_return['outer_bound_s']==60
assert q['boundaries']=={'native_runner_s':90,'process_s':140,'overall_s':250}
plan=direct_record(root/'plan.json')
for file,h in plan['files'].items():check_file(root/file,h)
helper=P('/private/tmp/protracker-mod-after-pcm-build-eqyyzl74/qualifier-current-v2/qualify_mod_output_safety_current.py');check_file(helper,'9ea1a8a6d08957a1bfcdcfaea70301e3bc61cf4abc35e9006ef93b120322d4e1')
assert outer['gate']['revision']==121 and outer['gate']['candidate_sha256']==r['binary_sha256']
assert 'timeout=250' in (root/'run_native_next_once.py').read_text() and 'timeout=60' in (root/'run_verify_return_once.py').read_text()
# Read-only scope and results only: no release or broader acceptance inferred.
report={'passed':True,'scope':'Independent host-only read-only audit of saved MOD native build/source/execution/cleanup records; no helpers or native binaries executed, no Guest/target/process/lock/browser/network queries, no repo mutation. Coordination/release remains root-reported only.', 'qualification':records[str(qual/'qualification-result.json')], 'qualification_path':str(qual/'qualification-result.json'),'current_commit':current_commit,'current_tree':current_tree,'candidate':{'bytes':len(binary),'sha256':sha(binary),'ordered_translation_units':units,'canonical_dependencies':23,'sdk_headers':74,'runtime_files':7,'build_commands_rc0':20,'frozen_source_files_verified':7633,'original_sdk_assert_macro':macro},'native':{'returncode':'0','fast_allocations':13,'zero_owned_bytes':True,'workspace':7164,'transactional_staging_clean':True,'owned_run_cleanup':True,'inner_cleanup_short_observations':len(native['cleanup_absence_observations']),'independent_observations':11,'independent_span_s':internal_span,'outer_elapsed_s':outer['elapsed_s'],'boundaries':q['boundaries']},'separate_return':{'path':str(root/'separate-idle-result.json'),'sha256':records[str(root/'separate-idle-result.json')]['sha256'],'observations':11,'span_s':separate_span,'outer_elapsed_s':outer_return['elapsed_s'],'original_process_profile_cpu_running_dmaoff':True},'coordination':{'root_reported_take_revision':121,'release_independently_verified':False,'note':'Historical outer execution record conservatively retains recovery_hold/release_not_performed flags; this audit does not reinterpret them as a final hold or release.'},'acceptance_exclusions':['physical','device/card/MMIO','audio/playback/listening','timing/endurance'],'records':records}
out=P(tempfile.mkdtemp(prefix='protracker-mod-native-record-audit-',dir='/private/tmp'));p=out/'audit.json';p.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'report':str(p),'bytes':p.stat().st_size,'sha256':sha(p.read_bytes()),'verified_records':len(records),'native_span_s':internal_span,'separate_span_s':separate_span,'current_commit':current_commit},indent=2))
