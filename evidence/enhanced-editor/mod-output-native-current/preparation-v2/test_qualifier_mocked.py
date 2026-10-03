"""Private host-only mocks; never import shared_guest or execute any target command."""
import contextlib,hashlib,importlib.util,io,json,pathlib,shutil,subprocess,tempfile,time,unittest
from unittest.mock import patch
BASE=pathlib.Path('/private/tmp/protracker-mod-after-pcm-build-eqyyzl74')
HELPER=pathlib.Path(__file__).with_name('qualify_mod_output_safety_current.py')
spec=importlib.util.spec_from_file_location('private_mod_qualifier',HELPER)
q=importlib.util.module_from_spec(spec);spec.loader.exec_module(q)
MANIFEST=json.loads((BASE/'native-final/manifest.json').read_text())
class MockOnly(unittest.TestCase):
 def check_output(self,argv,**kw):
  if argv[:2]==['git','rev-parse']:return 'MOCK-COMMITTED-TREE\n'
  if argv[:2]==['git','show']:
   relative=argv[2].split(':',1)[1]
   data=(BASE/'source'/relative).read_bytes()
   if getattr(self,'source_bad',False) and relative=='src/core/pcm.c':data+=b'\n/* altered mock */\n'
   return data
  if argv[0]==MANIFEST['compiler'] and argv[-1].startswith('-print-file-name='):
   name=argv[-1].split('=',1)[1];path=MANIFEST['runtime_paths'][name]
   if getattr(self,'runtime_lexical',False):
    p=pathlib.Path(path);path=str(p.parent/'..'/p.parent.name/p.name)
   if name=='libstubs.a' and getattr(self,'runtime_override',None):path=self.runtime_override
   return (path+'-changed' if getattr(self,'runtime_bad',False) else path)+'\n'
  raise AssertionError('Unmocked process invocation refused: '+repr(argv))
 def test_verify_only_is_target_free(self):
  called=[]
  with tempfile.TemporaryDirectory(prefix='mod-qualifier-mock-verify-') as td, patch.object(q.subprocess,'check_output',self.check_output), patch.object(q,'live_adapters',lambda:called.append('FORBIDDEN')):
   with contextlib.redirect_stdout(io.StringIO()) as out:q.main([str(BASE/'native-final'),'--repo',td,'--verify-only'])
   d=json.loads(out.getvalue());self.assertTrue(d['passed']);self.assertEqual(len(d['cases']),1)
   self.assertEqual(d['cases'][0]['label'],'mod-stream');self.assertEqual(d['cases'][0]['mode'],['--mod-stream'])
   self.assertEqual(called,[]);self.assertEqual(list(pathlib.Path(td).iterdir()),[])
 def test_changed_source_refuses(self):
  self.source_bad=True
  with patch.object(q.subprocess,'check_output',self.check_output):
   with self.assertRaisesRegex(RuntimeError,'Committed source changed'):q.verified(BASE/'native-final',BASE/'source')
 def test_changed_runtime_path_refuses(self):
  self.runtime_bad=True
  with patch.object(q.subprocess,'check_output',self.check_output):
   with self.assertRaisesRegex(RuntimeError,'Resolved runtime path changed'):q.verified(BASE/'native-final',BASE/'source')
 def test_lexical_runtime_path_same_canonical_file_passes(self):
  self.runtime_lexical=True
  with patch.object(q.subprocess,'check_output',self.check_output):
   tree,cases,_=q.verified(BASE/'native-final',BASE/'source')
  self.assertEqual(len(cases),1);self.assertEqual(tree,'MOCK-COMMITTED-TREE')
 def test_different_existing_runtime_same_bytes_refuses(self):
  with tempfile.TemporaryDirectory(prefix='mod-runtime-path-MOCK-') as td:
   p=pathlib.Path(td)/'libstubs.a';shutil.copyfile(MANIFEST['runtime_paths']['libstubs.a'],p)
   self.runtime_override=str(p)
   self.assertEqual(q.sha(p.read_bytes()),MANIFEST['runtime_inputs']['libstubs.a'])
   with patch.object(q.subprocess,'check_output',self.check_output):
    with self.assertRaisesRegex(RuntimeError,'Resolved runtime path changed: libstubs.a'):q.verified(BASE/'native-final',BASE/'source')
 def test_changed_candidate_before_guest(self):
  with tempfile.TemporaryDirectory(prefix='mod-qualifier-mock-binary-') as td:
   build=pathlib.Path(td);shutil.copyfile(BASE/'native-final/manifest.json',build/'manifest.json');shutil.copyfile(BASE/'native-final/PTExecModStreamTest',build/'PTExecModStreamTest');shutil.copyfile(BASE/'native-final/PTExecModStreamTest-preprocessed-wrapper.log',build/'PTExecModStreamTest-preprocessed-wrapper.log')
   with (build/'PTExecModStreamTest').open('ab') as f:f.write(b'X')
   with patch.object(q.subprocess,'check_output',self.check_output):
    with self.assertRaisesRegex(RuntimeError,'Pinned file changed'):q.verified(build,BASE/'source')
 def run_simulated(self,scenario):
  with tempfile.TemporaryDirectory(prefix='mod-qualifier-MOCK-ONLY-') as td:
   root=pathlib.Path(td);infra=root/'MOCK-INFRA';repo=root/'MOCK-REPO';repo.mkdir()
   profile=infra/'runtime/amiberry/Configurations/A1200-030-DEV.uae';profile.parent.mkdir(parents=True);profile.write_text('MOCK PROFILE, NOT A TARGET\n')
   (infra/'runtime/test.lock').touch();(infra/'runtime/Dev/Tests').mkdir(parents=True)
   identity={'process':[900001,'MOCK EXACT COMMAND, NEVER RUN'],'profile':str(profile),'profile_sha256':q.sha(profile.read_bytes()),'scope':'MOCK IDENTITY ONLY'}
   ip=root/'MOCK-identity.json';ip.write_text(json.dumps(identity))
   if scenario=='profile-changed':profile.write_text('MOCK REPLACED PROFILE, NOT A TARGET\n')
   now=[100.0];calls={'guest':0,'adapter':0,'runner':0,'processes':0,'sleeps':0};captured={}
   class FakeGuest:
    def __init__(self,infra_,out):
     calls['guest']+=1;self.share=infra_/'runtime/Dev/Tests';self.launch=self.share/('launch-'+out.name)
    def command(self,command):
     if command=='GET_STATUS':
      if scenario=='insufficient-window':now[0]+=200
      if scenario=='overall-deadline' and calls['processes']>=4:now[0]+=250
      return 'OK\tPaused=false'
     if command=='GET_CPU_MODEL':return 'OK\tmodel='+('68020' if scenario=='wrong-cpu' or (scenario=='wrong-final-cpu' and calls['processes']>=4) else '68030')
     if command=='GET_AUDIO_STATE':return 'OK\t'+'\t'.join('ch%d_dma=0'%i for i in range(4))
     raise AssertionError('Unmocked Guest command refused')
   def processes():
    calls['processes']+=1
    if scenario=='wrong-pid' or (scenario=='identity-lost-after-check' and calls['processes']>=4):return [(900002,identity['process'][1])]
    return [tuple(identity['process'])]
   def adapters():calls['adapter']+=1;return FakeGuest,processes
   class Archive:
    stdout=io.BytesIO(b'MOCK ARCHIVE')
    def __enter__(self):return self
    def __exit__(self,*a):return False
    def wait(self,timeout=None):return 0
    def poll(self):return 0
   def popen(argv,**kw):
    if argv[:2]!=['git','archive']:raise AssertionError('Unmocked Popen refused')
    return Archive()
   def run(argv,**kw):
    if argv[:3]==['tar','-x','-C']:
     shutil.copytree(BASE/'source/tools',pathlib.Path(argv[3])/'tools');return subprocess.CompletedProcess(argv,0)
    if argv==[str(infra/'.venv/bin/python'),'tools/shared_infra_render_files.py','--mod-stream']:
     self.assertEqual(kw['timeout'],140);calls['runner']+=1
     native=pathlib.Path(kw['cwd'])/'build/dev/render-files-900001';native.mkdir()
     record={'passed':True,'run_files_cleaned':True,'mod-stream_returncode':'0','sample_staging_clean':True,'PTExecModStreamTest_sha256':q.BINARY_SHA256,'cleanup_audio':'OK\t'+'\t'.join('ch%d_dma=0'%i for i in range(4)),'scope':'SIMULATED, NOT NATIVE EVIDENCE'}
     if scenario=='cleanup-refused':record['run_files_cleaned']=False
     if scenario=='staging-refused':record['sample_staging_clean']=False
     (native/'result.json').write_text(json.dumps(record));markers=['MOD STREAM PASS:','EXEC MEMORY PASS:','zero owned bytes']
     if scenario=='missing-extra-marker':markers=markers[1:]
     if scenario=='missing-allocator-zero':markers=markers[:2]
     (native/'mod-stream.log').write_text('MOCK ONLY\n'+'\n'.join(markers)+'\n');kw['stdout'].write(str(native)+'\n')
     captured['nested']=native
     return subprocess.CompletedProcess(argv,20 if scenario=='runner-failed' else 0)
    raise AssertionError('Unmocked subprocess.run refused: '+repr(argv))
   def sleep(seconds):
    self.assertEqual(seconds,1.001);calls['sleeps']+=1
    if scenario!='no-observed-time':now[0]+=seconds
   def lexists(path):return scenario=='path-reappeared' and str(path).endswith('render-files-900001')
   args=[str(BASE/'native-final'),'--repo',str(repo)]
   if scenario!='missing-identity':args+=['--identity',str(ip),'--identity-sha256',q.sha(ip.read_bytes())]
   err=None
   with patch.object(q,'INFRA',infra), patch.object(q.subprocess,'check_output',self.check_output),patch.object(q.subprocess,'Popen',popen),patch.object(q.subprocess,'run',run),patch.object(q,'live_adapters',adapters),patch.object(q.time,'monotonic',lambda:now[0]),patch.object(q.time,'sleep',sleep),patch.object(q.os.path,'lexists',lexists):
    with contextlib.redirect_stdout(io.StringIO()):
     try:q.main(args)
     except RuntimeError as e:err=str(e)
   reports=list((repo/'build/dev').glob('mod-output-safety-current-qualification-*/qualification-result.json'));self.assertEqual(len(reports),1)
   report=json.loads(reports[0].read_text());checked=None
   if 'nested' in captured and (captured['nested']/'independent-cleanup.json').exists():checked=json.loads((captured['nested']/'independent-cleanup.json').read_text())
   return err,report,checked,calls
 def test_success_is_simulated_only(self):
  err,d,i,c=self.run_simulated('success');self.assertIsNone(err);self.assertTrue(d['passed']);self.assertEqual(len(d['cases']),1)
  self.assertTrue(i['passed']);self.assertEqual(len(i['observations']),11);self.assertGreater(i['observed_elapsed_s'],10);self.assertEqual(c['sleeps'],10);self.assertEqual(c['runner'],1)
  self.assertEqual(d['boundaries'],{'native_runner_s':90,'process_s':140,'overall_s':250})
 def test_missing_identity_before_any_guest_or_staging(self):
  err,d,i,c=self.run_simulated('missing-identity');self.assertIn('Fresh explicitly pinned identity',err);self.assertFalse(d['passed']);self.assertEqual(d['cases'],[]);self.assertEqual(c['guest'],0);self.assertEqual(c['adapter'],0);self.assertEqual(c['runner'],0)
 def test_changed_profile_before_any_guest_or_staging(self):
  err,d,i,c=self.run_simulated('profile-changed');self.assertIn('Pinned file changed',err);self.assertFalse(d['passed']);self.assertEqual(c['guest'],0);self.assertEqual(c['adapter'],0);self.assertEqual(c['runner'],0)
 def test_fresh_cpu_guard_before_launch(self):
  err,d,i,c=self.run_simulated('wrong-cpu');self.assertIn('Running DMA-off guard',err);self.assertFalse(d['passed']);self.assertEqual(c['runner'],0)
 def test_wrong_original_pid_before_any_guest(self):
  err,d,i,c=self.run_simulated('wrong-pid');self.assertIn('Pinned original sole emulator changed',err);self.assertFalse(d['passed']);self.assertEqual(c['guest'],0);self.assertEqual(c['runner'],0)
 def test_insufficient_budget_prevents_launch(self):
  err,d,i,c=self.run_simulated('insufficient-window');self.assertIn('Insufficient bounded window',err);self.assertFalse(d['passed']);self.assertEqual(c['runner'],0)
 def test_runner_failure_preserves_top_no_retry(self):
  err,d,i,c=self.run_simulated('runner-failed');self.assertIn('runner failed',err);self.assertFalse(d['passed']);self.assertEqual(c['runner'],1);self.assertEqual(d['cases'],[])
 def test_cleanup_failure_preserves_top_no_retry(self):
  err,d,i,c=self.run_simulated('cleanup-refused');self.assertIn('pass/cleanup/hash',err);self.assertFalse(d['passed']);self.assertEqual(c['runner'],1);self.assertEqual(d['cases'],[])
 def test_extra_marker_required(self):
  err,d,i,c=self.run_simulated('missing-extra-marker');self.assertIn('All MOD fixture',err);self.assertFalse(d['passed']);self.assertEqual(d['cases'],[])
 def test_transactional_staging_guard_required(self):
  err,d,i,c=self.run_simulated('staging-refused');self.assertIn('sample staging cleanup',err);self.assertFalse(d['passed']);self.assertEqual(d['cases'],[]);self.assertEqual(c['runner'],1)
 def test_allocator_zero_marker_required(self):
  err,d,i,c=self.run_simulated('missing-allocator-zero');self.assertIn('All MOD fixture',err);self.assertFalse(d['passed']);self.assertEqual(d['cases'],[])
 def test_final_cpu_guard_required(self):
  err,d,i,c=self.run_simulated('wrong-final-cpu');self.assertIn('Running DMA-off guard',err);self.assertFalse(d['passed']);self.assertFalse(i['passed']);self.assertEqual(len(i['observations']),11)
 def test_reappearance_preserves_failed_independent_record(self):
  err,d,i,c=self.run_simulated('path-reappeared');self.assertIn('paths reappeared',err);self.assertFalse(d['passed']);self.assertFalse(i['passed']);self.assertEqual(len(i['observations']),1);self.assertIn('error',i);self.assertEqual(c['runner'],1)
 def test_minimum_observed_duration(self):
  err,d,i,c=self.run_simulated('no-observed-time');self.assertIn('11 observations over >10s',err);self.assertFalse(d['passed']);self.assertFalse(i['passed']);self.assertEqual(len(i['observations']),11)
 def test_changed_identity_after_absence_is_not_pass(self):
  err,d,i,c=self.run_simulated('identity-lost-after-check');self.assertIn('Pinned original',err);self.assertFalse(d['passed']);self.assertFalse(i['passed']);self.assertEqual(len(i['observations']),11);self.assertIn('error',i)
 def test_overall_limit_includes_final_cleanup(self):
  err,d,i,c=self.run_simulated('overall-deadline');self.assertIn('window exceeded',err);self.assertFalse(d['passed']);self.assertTrue(i['passed']);self.assertEqual(len(d['cases']),1)
if __name__=='__main__':
 started=time.monotonic();suite=unittest.defaultTestLoader.loadTestsFromTestCase(MockOnly)
 result=unittest.TextTestRunner(verbosity=2).run(suite)
 report={'scope':'Host-only mocked qualifier guards, all identities/Guest/IPC/processes/native runner/time/shared locks are simulated in private temporary directories; no real target/Guest/IPC/shared lock/native execution',
         'passed':result.wasSuccessful(),'tests':result.testsRun,'failures':len(result.failures),'errors':len(result.errors),'elapsed_s':time.monotonic()-started,
         'qualifier':{'path':str(HELPER),'bytes':HELPER.stat().st_size,'sha256':hashlib.sha256(HELPER.read_bytes()).hexdigest()},
         'mock_test':{'path':str(pathlib.Path(__file__)),'bytes':pathlib.Path(__file__).stat().st_size,'sha256':hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest()},
         'build_manifest_sha256':hashlib.sha256((BASE/'native-final/manifest.json').read_bytes()).hexdigest()}
 (pathlib.Path(__file__).with_name('mock-proof.json')).write_text(json.dumps(report,indent=2)+'\n')
 raise SystemExit(0 if result.wasSuccessful() else 1)
