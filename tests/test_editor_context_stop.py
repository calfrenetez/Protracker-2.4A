"""Private files and fake stop child/guard only; no process or target action."""
import json,os,subprocess,sys,time
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import editor_context_stop as stop
import editor_pointer_parent as original
import editor_context_volume as volume
import test_editor_context_lifecycle as fixtures
class StopSupervisorTests(unittest.TestCase):
 prepare=fixtures.ContextLifecycleTests.prepare
 def setUp(self):
  fixtures.ContextLifecycleTests.setUp(self)
  m=patch.object(stop,'HARNESS',self.h);m.start();self.addCleanup(m.stop)
  m=patch.dict(os.environ,{},clear=True);m.start();self.addCleanup(m.stop)
  session=Path(self.b['blankBinding']['modulePaths']['capture_runner']).parent
  helper=volume.put(session/'HOST-root-lifecycle.py',b'# HOST fake selected helper, never executed\n')
  context=volume.save(session/'ROOT-CURRENT-CONTEXT.json',{'state':'ROOT_REVIEWED_CONTEXT_V3_CURRENT_LIFECYCLE_CONTEXT','sessionDirectory':str(session),'windowDirectory':str(self.window),'runLabel':self.label})
  ready=volume.save(session/'HOST-READY.json',{'state':'ROOT_ACTUALLY_VERIFIED_HUMAN_APPROVED_DEVBENCH_READY','acceptedByRoot':True,'observed':{'job':{'pid':'555'},'processIdentity':'555 HOST_START HOST_SERVICE'},'HOST_SYNTHETIC':True})
  self.freeze['actualApprovedServiceReadinessRef']=ready
  service=volume.save(self.window/'ROOT_CURRENT_DEVBENCH_PROCESS.json',{'pid':555,'argv':['555','555','HOST_SERVICE'],'approvedStartIdentity':'555 HOST_START HOST_SERVICE'})
  for ref in (helper,context,ready,original.read(stop.__file__)[1]):self.freeze['sources'][ref['path']]=ref
  path=Path(self.b['sourceFreezeRef']['path']);path.write_text(json.dumps(self.freeze));freeze=original.read(path)[1]
  self.b['sourceFreezeRef']=self.p['sourceFreezeRef']=freeze
  path=Path(self.parent_ref['path']);path.write_text(json.dumps(self.b));self.parent_ref=original.read(path)[1]
  volume.save(self.window/'private-lease.json',{'lease':{'run_id':'fixture-owner','HOST_ONLY':True}})
  self.out.mkdir();self.result={'state':'EDITOR_CONTEXT_V3_DIAGNOSTIC_DATA_RECORDED_NO_INPUT','guiInputEvents':0,'hostRaises':0,'inputAuthorized':False,'selectionAccepted':False,'hotspotMappingAccepted':False,'contextObservation':{'state':'PASSIVE_EDITOR_V3_CONTEXT_DATA_RECORDED','readbackAttempts':1,'executionAttempts':1}}
  self.scope={'state':stop.STATE,'acceptedByRoot':True,'allowedActions':dict(stop.ALLOWED),'limits':{'supervisor':180},'ownerRunID':'fixture-owner','windowDirectory':str(self.window),'parentBindingRef':self.parent_ref,'contextRef':context,'lifecycleSourceRef':helper,'serviceProcessRef':service,'diagnosticResultRef':volume.save(self.out/'context-parent-result.json',self.result)}
  self.ref=volume.save(self.window/'normal-stop-supervisor-binding.json',self.scope)
  self.option=None;self.children=[];outer=self
  class Child:
   pid=888888
   returncode=0
   def wait(self,timeout):
    outer.assertEqual(timeout,180)
    if outer.option=='timeout':raise subprocess.TimeoutExpired('HOST child',timeout)
    if outer.option=='nonzero':self.returncode=1;return
    if outer.option=='no-receipt':return
    volume.save(outer.window/'ROOT_ACTUAL_OWNED_STOP.json',{'state':'ROOT_VERIFIED_FRESH_OWNED_VM_STOP_WITH_BRIDGE_DISCONNECTED','ownerRunID':'fixture-owner','mode':'normal','pid':999998 if outer.option=='wrong-pid' else outer.owned['pid'],'processAndGroupAbsent':True,'port2345Absent':True,'canonicalSocketAbsent':True,'guestCleanExitClaimed':False,'HOST_SYNTHETIC':True})
  def popen(argv,**kw):
   self.children.append((argv,kw));self.assertEqual(argv[3],'stop-normal');self.assertEqual(argv[4],'--context-ref');self.assertEqual(json.loads(argv[5]),context)
   for key in stop.AUTHORITY:self.assertNotIn(key,kw['env'])
   return Child()
  def command(argv,accepted):
   if self.option=='service':return 'OTHER'
   return '555 555 HOST_SERVICE' if argv[-1]=='pid=,pgid=,command=' else '555 HOST_START HOST_SERVICE'
  self.supervisor=SimpleNamespace(guard=self.d.guard,decode=json.loads,popen=popen,observations=SimpleNamespace(command=command))
 def test_correct_child_receives_no_inherited_authority_and_returns_receipt_once(self):
  result=stop.execute(self.ref,self.supervisor);self.assertEqual(result['returncode'],0);self.assertFalse(result['guardReleaseClaimed']);self.assertEqual(result['signals'],0)
  with self.assertRaises(RuntimeError):stop.execute(self.ref,self.supervisor)
  self.assertEqual(len(self.children),1)
 def test_inherited_authority_rejected_before_even_reading_records_or_intent(self):
  for key in stop.AUTHORITY:
   with patch.dict(os.environ,{key:'HOST_FOREIGN'}),patch.object(original,'pinned',side_effect=AssertionError('No record read allowed')):
    with self.assertRaisesRegex(RuntimeError,'before records or intent'):stop.execute({},self.supervisor)
  self.assertEqual(self.children,[]);self.assertFalse((self.window/'normal-stop-supervisor-intent.json').exists());self.assertFalse((self.window/'owned-stop-intent.json').exists())
 def test_existing_original_stop_intent_is_never_replayed(self):
  (self.window/'owned-stop-intent.json').write_bytes(b'HOST previous failed attempt')
  with self.assertRaises(RuntimeError):stop.execute(self.ref,self.supervisor)
  self.assertEqual(self.children,[])
 def test_changed_owner_prevents_child_and_supervisor_intent(self):
  self.condition='owner'
  with self.assertRaises(RuntimeError):stop.execute(self.ref,self.supervisor)
  self.assertEqual(self.children,[]);self.assertFalse((self.window/'normal-stop-supervisor-intent.json').exists())
 def test_hold_prevents_normal_stop_child(self):
  self.phase='HOLD'
  with self.assertRaises(RuntimeError):stop.execute(self.ref,self.supervisor)
  self.assertEqual(self.children,[])
 def test_nonzero_child_retained_without_signal_retry_or_release(self):
  self.option='nonzero'
  with self.assertRaises(RuntimeError):stop.execute(self.ref,self.supervisor)
  failure=json.loads((self.window/'normal-stop-supervisor-failed.json').read_text());self.assertEqual(failure['signals'],0);self.assertFalse(failure['guardReleaseClaimed'])
  with self.assertRaises(RuntimeError):stop.execute(self.ref,self.supervisor)
  self.assertEqual(len(self.children),1)
 def test_timed_out_child_retained_without_kill_or_repeat(self):
  self.option='timeout'
  with self.assertRaises(subprocess.TimeoutExpired):stop.execute(self.ref,self.supervisor)
  with self.assertRaises(RuntimeError):stop.execute(self.ref,self.supervisor)
  self.assertEqual(len(self.children),1);self.assertEqual(self.release_calls,0)
 def test_success_exit_without_receipt_is_not_stop_acceptance(self):
  self.option='no-receipt'
  with self.assertRaises(FileNotFoundError):stop.execute(self.ref,self.supervisor)
  self.assertTrue((self.window/'normal-stop-supervisor-failed.json').exists())
 def test_wrong_pid_receipt_is_not_accepted(self):
  self.option='wrong-pid'
  with self.assertRaises(RuntimeError):stop.execute(self.ref,self.supervisor)
  self.assertFalse((self.window/'normal-stop-supervisor-result.json').exists())
 def test_bad_diagnostic_result_prevents_stop(self):
  self.result['guiInputEvents']=1;path=Path(self.scope['diagnosticResultRef']['path']);path.write_text(json.dumps(self.result));self.scope['diagnosticResultRef']=original.read(path)[1]
  path=Path(self.ref['path']);path.write_text(json.dumps(self.scope));self.ref=original.read(path)[1]
  with self.assertRaises(RuntimeError):stop.execute(self.ref,self.supervisor)
  self.assertEqual(self.children,[])
 def test_changed_shared_service_prevents_original_stop_and_intent(self):
  self.option='service'
  with self.assertRaises(RuntimeError):stop.execute(self.ref,self.supervisor)
  self.assertEqual(self.children,[]);self.assertFalse((self.window/'normal-stop-supervisor-intent.json').exists())
 def test_alternate_runtime_refuses_before_project_imports(self):
  result=subprocess.run(['/usr/bin/python3','-B',str(Path(stop.__file__))],capture_output=True,text=True,timeout=10)
  self.assertNotEqual(result.returncode,0);self.assertIn('Original harness clock required before stop supervisor imports',result.stderr);self.assertNotIn('editor_pointer_parent.py',result.stderr)
if __name__=='__main__':unittest.main()
