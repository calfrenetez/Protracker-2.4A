"""One Exec MOD export/output-preservation fixture; live use requires a fresh separately owned shared030 window.

Preparation is NOT RUN. --verify-only checks committed inputs/toolchain without
Guest construction, lock acquisition, staging or target commands. A live call
requires an explicitly fingerprinted identity record supplied from its fresh
coordination window; this script never discovers or adopts a replacement PID.
"""
import argparse,fcntl,hashlib,json,os,re,shutil,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
MANIFEST_SHA256='4027777578e796af0eaa3e061559c98a022df2803ee90b170291ad3e58af36dc'
BINARY_SHA256='52e273bdf829092f1bad2182d1fa9e544e314a6d5dff81bb45880c947cdf8102'
RUNNER_SHA256='df9c7f37252bb27e9315aa244854db12a3186ba987924c0d094f5142ae968978'
RECIPE_SHA256='1948232ab42449eb518e49ee55276542b102d7461f0f58d8aa273588a20a0d55'
FLAGS=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core','-Ibuild/dev','-fbbb=-']
UNITS=['tests/native_exec_mod_stream_test.c','src/platform/mod_file.c','src/platform/file_save.c','src/core/safe_save.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
ASSERT_MACRO='#define assert(__e) ((__e) ? (void)0 : __assert_func (__FILE__, __LINE__, __ASSERT_FUNC, #__e))'
RUNTIMES={'ncrt0.o','libnix20.a','libnixmain.a','libnix.a','libstubs.a','libamiga.a','libgcc.a'}
SCOPE='One shared030 Exec MOD analysis/encode output-master boundary, transactional file stream and Fast-pool ownership fixture; original SDK __assert_func retained; no device/card/MMIO/physical/audio/timing/listening acceptance'

def sha(data):return hashlib.sha256(data).hexdigest()
def write(path,value):path.write_text(json.dumps(value,indent=2)+'\n')
def same_file(path,expected,bytes_=None):
 if not path.is_file() or path.is_symlink() or sha(path.read_bytes())!=expected or (bytes_ is not None and path.stat().st_size!=bytes_):
  raise RuntimeError('Pinned file changed or absent: '+str(path))
def committed(repo,tree,path):
 if Path(path).is_absolute() or '..' in Path(path).parts:raise RuntimeError('Noncanonical source path')
 return subprocess.check_output(['git','show',tree+':'+path],cwd=repo,timeout=10)
def verified(build,repo):
 same_file(build/'manifest.json',MANIFEST_SHA256)
 manifest=json.loads((build/'manifest.json').read_text())
 if not manifest.get('passed') or set(manifest.get('targets',{}))!={'PTExecModStreamTest'}:raise RuntimeError('Exactly one successful scoped build required')
 tree=subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=repo,text=True,timeout=10).strip()
 record=manifest['targets']['PTExecModStreamTest']
 if record['source_inputs']!=UNITS or record['flags']!=FLAGS or manifest['flags']!=FLAGS:raise RuntimeError('Canonical MOD recipe changed')
 if record['binary_sha256']!=BINARY_SHA256 or record['binary_bytes']!=70824:raise RuntimeError('Unexpected MOD candidate')
 binary=build/'PTExecModStreamTest';same_file(binary,BINARY_SHA256,70824)
 if binary.read_bytes()[:4]!=bytes.fromhex('000003f3'):raise RuntimeError('HUNK binary header changed')
 if len(record['dependencies'])!=23 or len(record['external_sdk_dependencies'])!=74:raise RuntimeError('Exact MOD compile closure changed')
 pp=record['preprocessed_wrapper'];same_file(build/'PTExecModStreamTest-preprocessed-wrapper.log',pp['sha256'],pp['bytes'])
 macros=[line for line in (build/'PTExecModStreamTest-preprocessed-wrapper.log').read_text().splitlines() if line.startswith('#define assert(')]
 if record.get('last_assert_macro')!=ASSERT_MACRO or not macros or macros[-1]!=ASSERT_MACRO:raise RuntimeError('Original literal SDK __assert_func macro changed')
 for path,expected in record['dependencies'].items():
  if sha(committed(repo,tree,path))!=expected:raise RuntimeError('Committed source changed: '+path)
 for path,expected in record['external_sdk_dependencies'].items():same_file(Path(path),expected)
 if manifest['canonical_recipe']['sha256']!=RECIPE_SHA256 or sha(committed(repo,tree,'tools/build_core_tests.py'))!=RECIPE_SHA256:
  raise RuntimeError('Committed canonical recipe changed')
 for path,expected in manifest['python_declaration_inputs'].items():
  if sha(committed(repo,tree,path))!=expected:raise RuntimeError('Committed recipe declaration changed: '+path)
 if sha(committed(repo,tree,'tools/shared_infra_render_files.py'))!=RUNNER_SHA256:raise RuntimeError('Committed runner changed')
 cc=Path(manifest['compiler']);same_file(cc,manifest['compiler_sha256'])
 if set(manifest['runtime_inputs'])!=RUNTIMES:raise RuntimeError('Exact seven runtime inputs required')
 for name,expected in manifest['runtime_inputs'].items():
  raw_path=Path(subprocess.check_output([str(cc),'-m68000','-msoft-float','-mcrt=nix20','-print-file-name='+name],text=True,timeout=10).strip())
  try:path=raw_path.resolve(strict=True)
  except (OSError,RuntimeError) as error:raise RuntimeError('Resolved runtime path changed: '+name) from error
  if str(path)!=manifest['runtime_paths'][name]:raise RuntimeError('Resolved runtime path changed: '+name)
  same_file(path,expected)
 case={'label':'mod-stream','binary':'PTExecModStreamTest','path':str(binary),'bytes':70824,'sha256':BINARY_SHA256,
       'markers':['MOD STREAM PASS:','EXEC MEMORY PASS:','zero owned bytes'],'mode':['--mod-stream'],'process_bound':140}
 return tree,[case],manifest

def load_identity(path,expected):
 if not path or not expected:raise RuntimeError('Fresh explicitly pinned identity required before live Guest or staging')
 same_file(path,expected);d=json.loads(path.read_text());process=d.get('process')
 profile=INFRA/'runtime/amiberry/Configurations/A1200-030-DEV.uae'
 if (not isinstance(process,list) or len(process)!=2 or not isinstance(process[0],int) or isinstance(process[0],bool) or process[0]<=0 or not isinstance(process[1],str) or not process[1] or d.get('profile')!=str(profile)):
  raise RuntimeError('Fresh shared030 process/profile identity malformed')
 same_file(profile,d['profile_sha256'])
 return d

def live_adapters():
 sys.path.insert(0,str(INFRA/'scripts'))
 from shared_guest import Guest
 from emulator import processes
 return Guest,processes

def require_identity(identity,processes):
 rows=processes()
 if len(rows)!=1 or list(rows[0])!=identity['process']:raise RuntimeError('Pinned original sole emulator changed; no replacement adopted')
 same_file(Path(identity['profile']),identity['profile_sha256'])
 return rows

def idle(guest):
 status=guest.command('GET_STATUS');audio=guest.command('GET_AUDIO_STATE');cpu=guest.command('GET_CPU_MODEL')
 if [s for s in status.split('\t') if s.startswith('Paused=')]!=['Paused=false'] or not all('ch%d_dma=0'%i in audio.split('\t') for i in range(4)) or 'model=68030' not in cpu.split('\t'):
  raise RuntimeError('Running DMA-off guard failed')
 return {'status':status,'audio':audio,'cpu':cpu}

def independent(run,identity,Guest,processes):
 report={'passed':False,'processes':[], 'observations':[], 'observation_elapsed_s':[]}
 started=time.monotonic();destination=run/'independent-cleanup.json';write(destination,report)
 try:
  with (INFRA/'runtime/test.lock').open('a') as lock:
   fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
   report['processes']=require_identity(identity,processes)
   guest=Guest(INFRA,run);report.update(idle(guest));write(destination,report)
   for i in range(11):
    paths={str(path):os.path.lexists(path) for path in (guest.share/run.name,guest.launch)}
    report['observations'].append(paths);report['observation_elapsed_s'].append(time.monotonic()-started)
    report['observed_elapsed_s']=time.monotonic()-started;write(destination,report)
    if any(paths.values()):raise RuntimeError('Independent exact paths reappeared; no retry')
    if i<10:time.sleep(1.001)
   if len(report['observations'])!=11 or report['observed_elapsed_s']<=10:raise RuntimeError('Independent 11 observations over >10s unconfirmed')
   require_identity(identity,processes);report['final_idle']=idle(guest);report['passed']=True
 except Exception as error:
  report['error']=str(error);raise
 finally:
  report['elapsed_s']=time.monotonic()-started;write(destination,report)
 return report

def main(argv=None):
 deadline=time.monotonic()+250
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('build',type=Path);parser.add_argument('--repo',type=Path,default=ROOT)
 parser.add_argument('--verify-only',action='store_true');parser.add_argument('--identity',type=Path);parser.add_argument('--identity-sha256')
 args=parser.parse_args(argv);repo=args.repo.resolve();build=args.build.resolve()
 if args.verify_only:
  tree,cases,_=verified(build,repo)
  print(json.dumps({'passed':True,'scope':'Host-only exact committed-source/compiler/runtime verification; no Guest/lock/staging/target','source_tree':tree,'cases':cases},indent=2));return
 out=repo/'build/dev'/('mod-output-safety-current-qualification-'+str(time.time_ns()));out.mkdir(parents=True)
 result={'passed':False,'scope':SCOPE,'cases':[],'boundaries':{'native_runner_s':90,'process_s':140,'overall_s':250}}
 print('EVIDENCE',out,flush=True)
 try:
  tree,cases,manifest=verified(build,repo);identity=load_identity(args.identity,args.identity_sha256)
  result.update(source_tree=tree,verified_manifest=manifest,identity_record=identity,identity_sha256=args.identity_sha256)
  runners={}
  for case in cases:
   runner=out/('runner-'+case['label']);runner.mkdir()
   export=subprocess.Popen(['git','archive',tree,'tools'],cwd=repo,stdout=subprocess.PIPE)
   try:
    subprocess.run(['tar','-x','-C',str(runner)],stdin=export.stdout,check=True,timeout=10)
    export.stdout.close()
    if export.wait(timeout=10):raise RuntimeError('Runner export failed')
   finally:
    export.stdout.close()
    if export.poll() is None:
     # This is only the host git-archive child, never a guest/runner process.
     export.kill();export.wait(timeout=5)
   same_file(runner/'tools/shared_infra_render_files.py',RUNNER_SHA256)
   artifacts=runner/'build/dev';artifacts.mkdir(parents=True)
   shutil.copyfile(case['path'],artifacts/case['binary']);same_file(artifacts/case['binary'],case['sha256'],case['bytes'])
   runners[case['label']]=runner
  Guest,processes=live_adapters()
  with (INFRA/'runtime/test.lock').open('a') as lock:
   fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB);rows=require_identity(identity,processes)
   result['original_processes']=rows;result['initial_idle']=idle(Guest(INFRA,out))
  for case in cases:
   if deadline-time.monotonic()<185:raise RuntimeError('Insufficient bounded window; no launch')
   with (INFRA/'runtime/test.lock').open('a') as lock:
    fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB);require_identity(identity,processes);idle(Guest(INFRA,out))
   if deadline-time.monotonic()<185:raise RuntimeError('Insufficient bounded window after live guards; no launch')
   runner=runners[case['label']]
   with (out/(case['label']+'-runner.log')).open('w') as log:
    rc=subprocess.run([str(INFRA/'.venv/bin/python'),'tools/shared_infra_render_files.py',*case['mode']],cwd=runner,stdout=log,stderr=subprocess.STDOUT,timeout=case['process_bound']).returncode
   raw=(out/(case['label']+'-runner.log')).read_text()
   if rc:raise RuntimeError('mod-stream runner failed; preserve evidence and stop')
   run=Path(raw.strip().splitlines()[-1]).resolve()
   if run.parent!=(runner/'build/dev').resolve() or not re.fullmatch(r'render-files-[0-9]+',run.name):raise RuntimeError('Runner result path escaped isolated evidence directory')
   record=json.loads((run/'result.json').read_text())
   if not record.get('passed') or not record.get('run_files_cleaned') or record.get('mod-stream_returncode')!='0' or record.get('PTExecModStreamTest_sha256')!=case['sha256']:
    raise RuntimeError('Native pass/cleanup/hash unconfirmed')
   if not record.get('sample_staging_clean'):raise RuntimeError('Transactional sample staging cleanup unconfirmed')
   if not all('ch%d_dma=0'%i in record.get('cleanup_audio','').split('\t') for i in range(4)):
    raise RuntimeError('Fresh cleanup DMA-off guard unconfirmed')
   native_log=(run/'mod-stream.log').read_text()
   if not all(marker in native_log for marker in case['markers']):raise RuntimeError('All MOD fixture/allocator-zero markers required')
   checked=independent(run,identity,Guest,processes)
   result['cases'].append({'candidate':case,'run':str(run),'result':record,'independent_cleanup':checked,'native_log':native_log})
   write(out/'qualification-result.json',result);print('mod-stream PASS',run,flush=True)
  if time.monotonic()>deadline:raise RuntimeError('Qualification window exceeded after guarded cleanup')
  result['passed']=True
 except Exception as error:
  result['error']=str(error);raise
 finally:
  result['elapsed_s']=250-(deadline-time.monotonic());write(out/'qualification-result.json',result);print(out,flush=True)
if __name__=='__main__':main()
