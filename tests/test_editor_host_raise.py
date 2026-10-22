"""Private HOST files and synthetic owner/UI; no original harness/UI calls."""
import asyncio
from contextlib import contextmanager
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import editor_host_raise as gate


class HostRaiseTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name);self.out=self.root/'host-window-raise'
        self.b={'state':gate.STATE,'acceptedByRoot':True,'allowedActions':dict(gate.ALLOWED),
            'wholeSeconds':60,'ownerRunID':'HOST-owner','appPath':'/HOST/Amiberry.app',
            'windowTitle':'Amiberry - [HOST-030]','sourceFreezeRef':{'path':'HOST-freeze'}}
        self.process={'pid':999999,'socket_inode':123,'argv':['HOST_VM']}
        self.now=100.;self.events=[];self.current_calls=0;self.failure=None
        self.change_owner=False;self.change_receipt=None;self.expired=False
        self.in_locks=False
        self.api=self

    @contextmanager
    def original_locks(self):
        self.events.append('locks');self.in_locks=True
        try:yield
        finally:self.in_locks=False;self.events.append('unlock')

    def current(self,binding):
        self.assertTrue(self.in_locks);self.current_calls+=1;self.events.append('current')
        if self.change_owner and self.current_calls>1:raise RuntimeError('HOST changed ownership')
        return copy.deepcopy(self.process)

    def clock(self):return self.now

    def save(self,path,value):
        raw=json.dumps(value,sort_keys=True).encode()
        with path.open('xb') as stream:stream.write(raw)
        return {'path':str(path),'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}

    def pinned(self,ref):
        raw=Path(ref['path']).read_bytes()
        if len(raw)!=ref['bytes'] or hashlib.sha256(raw).hexdigest()!=ref['sha256']:
            raise RuntimeError('HOST changed pinned evidence')
        return raw

    async def root_observation(self,request,ref):
        self.assertTrue(self.in_locks);self.events.append('HOST-root-observation')
        self.now=161 if self.expired else 102
        value={'state':'ROOT_ACTUALLY_OBSERVED_ONE_NATIVE_WINDOW_RAISE',
            'actuallyPerformedNativeRaise':True,'requestSHA256':ref['sha256'],
            'ownerRunID':request['ownerRunID'],'process':request['process'],
            'appPath':request['appPath'],'windowTitle':request['windowTitle'],
            'action':'Raise','actionCount':1,'guestMoves':0,'guestClicks':0,'keys':0,
            'observedMonotonic':101}
        for name in ('beforeSnapshot','actionResult','afterSnapshot'):
            value[name]=self.save(self.root/(name+'.json'),{'HOST_ONLY':name})
        if self.change_receipt:self.change_receipt(value,ref)
        return value

    def fail(self,binding,reason):
        self.assertTrue(self.in_locks);self.failure=reason;self.events.append('HOST-HOLD')

    def run_gate(self):return asyncio.run(gate.run(self.b,self.out,self))

    def test_one_root_observation_under_locks_never_accepts_guest_input(self):
        result=self.run_gate()
        self.assertEqual(result['state'],'ONE_ROOT_OBSERVED_HOST_WINDOW_RAISE_RECORDED')
        self.assertFalse(result['nativeInputReadinessAccepted'])
        self.assertFalse(result['guestInputAuthorized'])
        self.assertFalse(result['retryAllowed'])
        self.assertTrue(result['freshRegisteredCaptureBeforeGuestInputStillRequired'])
        self.assertEqual(self.events,['locks','current','HOST-root-observation','current','current','unlock'])

    def test_second_execution_refused_before_locks_or_root(self):
        self.run_gate();count=len(self.events)
        with self.assertRaisesRegex(RuntimeError,'Never replay'):self.run_gate()
        self.assertEqual(len(self.events),count)

    def test_changed_ownership_latches_failure_and_retains_request(self):
        self.change_owner=True
        with self.assertRaisesRegex(RuntimeError,'changed ownership'):self.run_gate()
        self.assertIsNotNone(self.failure)
        self.assertTrue((self.out/'request.json').exists())
        self.assertEqual(json.loads((self.out/'result.json').read_text())['state'],
            'HOST_WINDOW_RAISE_GATE_FAILED_RETAINED')

    def test_expired_observation_is_not_authority(self):
        self.expired=True
        with self.assertRaises(RuntimeError):self.run_gate()
        self.assertIsNotNone(self.failure)

    def test_different_owner_pid_or_app_refused(self):
        for name,value in [('ownerRunID','other'),('process',{'pid':7}),('appPath','/OTHER.app'),
                           ('windowTitle','Other window'),('requestSHA256','other')]:
            with self.subTest(name=name):
                self.out=self.root/('case-'+name)
                self.change_receipt=lambda receipt,ref:receipt.update({name:value})
                # Each synthetic fixture owns separate before/action/after files.
                for path in self.root.glob('*Snapshot.json'):path.unlink()
                (self.root/'actionResult.json').unlink(missing_ok=True)
                with self.assertRaises(RuntimeError):self.run_gate()

    def test_guest_input_or_multiple_raise_claim_refused(self):
        self.change_receipt=lambda value,ref:value.update(guestClicks=1)
        with self.assertRaises(RuntimeError):self.run_gate()
        self.assertIsNotNone(self.failure)

    def test_tampered_snapshot_refused(self):
        def tamper(value,ref):Path(value['beforeSnapshot']['path']).write_bytes(b'changed')
        self.change_receipt=tamper
        with self.assertRaisesRegex(RuntimeError,'changed pinned'):self.run_gate()

    def test_request_cannot_change_while_root_waits(self):
        def tamper(value,ref):Path(ref['path']).write_bytes(b'changed')
        self.change_receipt=tamper
        with self.assertRaisesRegex(RuntimeError,'changed pinned'):self.run_gate()

    def test_distinct_admission_no_implicit_extra_input_scope(self):
        self.b['allowedActions']['guestClicks']=1
        with self.assertRaisesRegex(RuntimeError,'Distinct'):self.run_gate()
        self.assertEqual(self.events,[])
        self.assertFalse(self.out.exists())

    def test_completed_evidence_is_bound_to_fresh_owner_process_freeze_and_window(self):
        result=self.run_gate();ref=self.save(self.out/'HOST-copy.json',result)
        ref['path']=str(self.out/'result.json')
        self.assertEqual(gate.verify_completed(ref,self.b['sourceFreezeRef'],'HOST-owner',
            self.process,self.root,self.pinned,103)['state'],result['state'])
        for owner,process,freeze,now in [('other',self.process,self.b['sourceFreezeRef'],103),
            ('HOST-owner',{'pid':1},self.b['sourceFreezeRef'],103),
            ('HOST-owner',self.process,{'path':'other'},103),
            ('HOST-owner',self.process,self.b['sourceFreezeRef'],163)]:
            with self.subTest(owner=owner,now=now):
                with self.assertRaises(RuntimeError):
                    gate.verify_completed(ref,freeze,owner,process,self.root,self.pinned,now)

    def test_selected_binder_gate_is_mandatory_and_unselected_receipt_refused(self):
        import editor_pointer_binding as binder
        path=str(binder.parent.TOOLS/'editor_host_raise.py')
        from unittest.mock import patch
        with patch.object(binder,'checked_module',return_value=gate):
            with self.assertRaisesRegex(RuntimeError,'must complete'):
                binder.require_selected_host_raise({}, {'sources':{path:{}}},'HOST-owner',self.process)
        with self.assertRaisesRegex(RuntimeError,'Unselected'):
            binder.require_selected_host_raise({'hostRaiseResultRef':{}},{'sources':{}},'HOST-owner',self.process)
        binder.require_selected_host_raise({}, {'sources':{}},'HOST-owner',self.process)

    def test_selected_binder_consumes_actual_completed_file_checks(self):
        import editor_pointer_binding as binder
        from unittest.mock import patch
        result=self.run_gate();ref=self.save(self.out/'HOST-copy.json',result)
        ref['path']=str(self.out/'result.json')
        record={'hostRaiseResultRef':ref,'sourceFreezeRef':self.b['sourceFreezeRef'],
                'windowDirectory':str(self.root)}
        path=str(binder.parent.TOOLS/'editor_host_raise.py')
        with patch.object(binder,'checked_module',return_value=gate), \
             patch.object(binder.parent,'pinned',side_effect=self.pinned), \
             patch.object(binder.time,'monotonic',return_value=103):
            binder.require_selected_host_raise(record,{'sources':{path:{}}},'HOST-owner',self.process)


if __name__=='__main__':unittest.main()
