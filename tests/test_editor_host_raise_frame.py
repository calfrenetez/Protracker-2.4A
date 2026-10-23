"""HOST same-FRAME observers/recapture and private original-guard closure.

All service, process, UI, editor launch and stop observations are stand-ins.
No original shared target, lock, lifecycle or network call is permitted.
"""
import asyncio
import copy
from contextlib import contextmanager
import hashlib
import json
import fcntl
import os
from pathlib import Path
import sys
import tempfile
from types import ModuleType,SimpleNamespace
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import editor_host_raise as gate
import editor_pointer_parent as parent
import editor_pointer_binding as binder
import editor_readonly_probe as ro
import editor_capture_settlement as full
import editor_probe_volume as volume
from test_editor_mouse_probe_case import picture,FakeAPI
import test_editor_readonly_lifecycle as lifecycle_fixture


class SameFrameObserverTests(unittest.TestCase):
    def setUp(self):
        self.events=[];self.frame={'nonce':'HOST-original-nonce'}
        self.blank={};self.original=SimpleNamespace(binding=self.blank,frame=self.frame,
            owner=lambda owner:self.events.append('original-owner'))
        self.binding={'blankBinding':self.blank,'ownerRunID':'HOST-owner','passiveProbe':{},
            'sourceFreezeRef':{'path':'HOST-freeze'},'serviceReadinessRef':{'path':'HOST-ready'},
            'sourcePins':{'HOST-source':{}},'runLabel':'blank-capture-firstmouse-r1'}
        self.process={'pid':999999,'socket_inode':17};self.blank['ownedProcess']=self.process
        self.admission={'ownerRunID':'HOST-owner','sourceFreezeRef':self.binding['sourceFreezeRef'],
            'operationNonceSHA256':hashlib.sha256(self.frame['nonce'].encode()).hexdigest()}
        self.observed={'job':{'pid':'555'},'processIdentity':'HOST-epoch','listenerPID':555,
            'noRecoveryActivation':True,'serial':{'connected':True,'host':'127.0.0.1','port':2345}}
        self.ready={'observed':copy.deepcopy(self.observed)};self.baseline=FakeAPI().baseline
        self.api=gate.SameFrameAPI(self.binding,self.original,self.frame,77,self.baseline)
        self.api.checked=lambda fd:self.events.append(('command-fd',fd))
        def pin(ref):
            if ref.get('path')=='HOST-freeze':return json.dumps({'extraModulePaths':{'service_readiness':'HOST-service-source'}}).encode()
            if ref.get('path')=='HOST-ready':return json.dumps(self.ready).encode()
            return b'HOST-source'
        self.api.pinned=pin
        for p in (patch.object(ro,'require_current_mounted_input',side_effect=lambda *a:self.events.append('paired-profile')),
            patch.object(binder,'checked_module',return_value=SimpleNamespace(observe_current=lambda:self.observed)),
            patch.object(parent,'read',return_value=(self.baseline,{}))):
            p.start();self.addCleanup(p.stop)

    def test_original_owner_frame_paired_profile_service_and_command_fd_required(self):
        self.assertEqual(self.api.current(self.admission),self.process)
        self.assertIn('original-owner',self.events);self.assertIn('paired-profile',self.events)
        self.assertEqual(self.events[0],('command-fd',77));self.assertEqual(self.events[-1],('command-fd',77))

    def test_foreign_frame_refuses_before_original_owner(self):
        self.original.frame={}
        with self.assertRaises(RuntimeError):self.api.current(self.admission)
        self.assertNotIn('original-owner',self.events)

    def test_changed_owner_nonce_or_freeze_refused(self):
        for key,value in [('ownerRunID','other'),('operationNonceSHA256','b'*64),('sourceFreezeRef',{})]:
            with self.subTest(key=key),self.assertRaises(RuntimeError):
                self.api.current(dict(self.admission,**{key:value}))

    def test_changed_service_epoch_refused(self):
        self.observed['processIdentity']='other'
        with self.assertRaises(RuntimeError):self.api.current(self.admission)

    def test_changed_endpoint_listener_or_recovery_refused(self):
        for key,value in [('serial',{'connected':False,'host':'127.0.0.1','port':2345}),
                          ('listenerPID',556),('noRecoveryActivation',False)]:
            with self.subTest(key=key):
                before=self.observed[key];self.observed[key]=value
                with self.assertRaises(RuntimeError):self.api.current(self.admission)
                self.observed[key]=before

    def test_changed_source_pin_refused(self):
        def refused(ref):raise RuntimeError('HOST source drift')
        self.api.pinned=refused
        with self.assertRaisesRegex(RuntimeError,'source drift'):self.api.current(self.admission)

    def test_changed_blank_log_refused(self):
        with patch.object(parent,'read',return_value=(b'HOST changed log',{})),self.assertRaises(ValueError):
            self.api.current(self.admission)

    def private_locks(self):
        import editor_probe_namespace as namespace
        tmp=tempfile.TemporaryDirectory();self.addCleanup(tmp.cleanup)
        h=Path(tmp.name);(h/'runtime').mkdir()
        for name in ('test.lock','launcher-transport.lock'):(h/'runtime'/name).touch(mode=0o600)
        for p in (patch.object(parent,'HARNESS',h),patch.object(namespace,'HARNESS',h)):
            p.start();self.addCleanup(p.stop)
        self.api.checked=namespace.checked_command_lock
        return h

    def test_existing_command_fd_and_transport_lock_remain_held(self):
        h=self.private_locks()
        with parent.command_lock(h/'runtime/test.lock') as fd:
            self.api.lock_fd=fd
            with self.api.original_locks():
                for name in ('test.lock','launcher-transport.lock'):
                    other=os.open(h/'runtime'/name,os.O_RDWR)
                    try:
                        with self.assertRaises(BlockingIOError):fcntl.flock(other,fcntl.LOCK_EX|fcntl.LOCK_NB)
                    finally:os.close(other)
            os.fstat(fd)  # The inherited original FD is never closed here.

    def test_busy_transport_is_not_bypassed_and_original_fd_retained(self):
        h=self.private_locks()
        with parent.command_lock(h/'runtime/test.lock') as fd, parent.command_lock(h/'runtime/launcher-transport.lock'):
            self.api.lock_fd=fd
            with self.assertRaises(BlockingIOError):
                with self.api.original_locks():self.fail('Busy transport bypassed')
            os.fstat(fd)

    def test_wrong_command_fd_refuses_before_transport(self):
        h=self.private_locks()
        with parent.command_lock(h/'runtime/launcher-transport.lock') as fd:
            self.api.lock_fd=fd
            with self.assertRaises(RuntimeError):
                with self.api.original_locks():self.fail('Wrong inherited FD accepted')


class SameFrameRecaptureTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name);self.case=self.root/'case';self.case.mkdir()
        self.label='blank-capture-firstmouse-r1';self.baseline=FakeAPI().baseline
        (self.case/'editor-after-capture.log').write_bytes(self.baseline)
        self.h=self.root/'harness';self.run=self.h/'runtime/Dev/Tests'/self.label;self.run.mkdir(parents=True)
        (self.run/'editor.log').write_bytes(self.baseline)
        self.b={'state':parent.READONLY_STATE,'outputDirectory':str(self.case),'runLabel':self.label,
            'ownerRunID':'HOST-owner','sourceFreezeRef':{'path':'HOST-freeze'}}
        self.blank={'state':'BLANK_LOG_AND_REGISTERED_CAPTURE_RECORDED',
            'logSHA256':hashlib.sha256(self.baseline).hexdigest(),'capture':picture(1),
            'attempts':{'candidate-readback':1,'environment-setup':1,'application-launch':1,'capture':1}}
        self.events=[];self.next_capture=picture(2);self.error=False;self.drift=False
        outer=self
        class API:
            APP='/HOST/Amiberry.app';TITLE='HOST-030'
            def current(self,a):outer.events.append('current')
        async def raised(a,out,api):
            self.events.append('raise');out.mkdir()
            if self.error:raise RuntimeError('HOST Root Raise refused')
        async def capture(path):
            self.events.append('registered-capture')
            if self.drift:(self.run/'editor.log').write_bytes(b'HOST changed')
            return self.next_capture
        self.original=SimpleNamespace(capture=capture);self.frame={'nonce':'HOST-nonce'}
        for p in (patch.object(gate,'SameFrameAPI',return_value=API()),patch.object(gate,'run',side_effect=raised),
                  patch.object(parent,'HARNESS',self.h)):
            p.start();self.addCleanup(p.stop)
    def run_case(self):return asyncio.run(gate.run_same_frame(self.b,self.blank,self.original,self.frame,77))
    def test_raise_then_fresh_capture_before_returning_mouse_capture(self):
        result=self.run_case()
        self.assertEqual(result['capture'],picture(2));self.assertEqual(self.blank['capture'],picture(1))
        self.assertEqual(self.events,['raise','current','registered-capture','current'])
        self.assertEqual(result['attempts'],self.blank['attempts'])
        value=json.loads((self.case/'host-window-raise/post-raise-capture.json').read_text())
        self.assertFalse(value['nativeInputReadinessAccepted']);self.assertFalse(value['guestInputAuthorized'])
    def test_raise_failure_prevents_recapture(self):
        self.error=True
        with self.assertRaisesRegex(RuntimeError,'Raise refused'):self.run_case()
        self.assertEqual(self.events,['raise'])
    def test_stale_capture_refused(self):
        self.next_capture=picture(1)
        with self.assertRaises(RuntimeError):self.run_case()
    def test_foreign_controller_refused(self):
        self.next_capture['controllerID']='other'
        with self.assertRaises(RuntimeError):self.run_case()
    def test_guest_log_drift_prevents_mouse_return(self):
        self.drift=True
        with self.assertRaises(ValueError):self.run_case()
        self.assertFalse((self.case/'host-window-raise/post-raise-capture.json').exists())

    def actual_image_observer(self):
        # Byte-for-byte copies of the validators used by the failed live run.
        # Loading their pure code here does not import the shared harness.
        fixtures=Path(__file__).parent/'fixtures/editor_observation'
        support=ModuleType('capture_support')
        source=(fixtures/'capture_support.py').read_bytes()
        self.assertEqual(hashlib.sha256(source).hexdigest(),
                         'd50a82781815ced87a33238dc5d59bb31915cbde90a185bc0c8489723a93f1ed')
        exec(compile(source,str(fixtures/'capture_support.py'),'exec'),support.__dict__)
        observer=ModuleType('actual_root_image_observer')
        source=(fixtures/'root_observation.py').read_bytes()
        self.assertEqual(hashlib.sha256(source).hexdigest(),
                         '891944870f007288f823926669dcc9545514071bb579667b595fdf80b63dfa8f')
        with patch.dict(sys.modules,{'capture_support':support}):
            exec(compile(source,str(fixtures/'root_observation.py'),'exec'),observer.__dict__)
        return observer

    def image_request(self,capture):
        return {'state':'ROOT_EDITOR_MOUSE_IMAGE_OBSERVATION_REQUIRED',
            'stage':'before-move','caseDirectory':str(self.case),
            'observationPath':str(self.case/'mouse-phase/before-move-observation.json'),
            'capture':capture,'requestedMonotonic':100.,'deadlineMonotonic':160.}

    def test_actual_recapture_path_passes_unchanged_root_image_observer(self):
        async def capture(path):
            value=copy.deepcopy(self.next_capture)
            path.write_bytes(b'\x89PNG\r\n\x1a\nHOST-image')
            value['image']=parent.read(path)[1]
            return value
        self.original.capture=capture
        result=self.run_case()
        observer=self.actual_image_observer()
        request=self.image_request(result['capture'])
        ref={'path':str(self.case/'mouse-phase/before-move-request.json')}
        self.assertIs(observer.validate_request(request,ref),request)

    def test_original_observer_still_refuses_private_gate_image_directory(self):
        observer=self.actual_image_observer()
        request=self.image_request({'image':{'path':str(self.case/'host-window-raise/after-raise.png')}})
        ref={'path':str(self.case/'mouse-phase/before-move-request.json')}
        with self.assertRaisesRegex(RuntimeError,'case/image/deadline'):
            observer.validate_request(request,ref)


class ParentRaiseIntegrationTests(unittest.TestCase):
    prepare=lifecycle_fixture.ReadonlyLifecycleTests.prepare
    setUp=lifecycle_fixture.ReadonlyLifecycleTests.setUp
    bind=lifecycle_fixture.ReadonlyLifecycleTests.bind
    full_binding=lifecycle_fixture.ReadonlyLifecycleTests.full_binding
    real_paired_originals=lifecycle_fixture.ReadonlyLifecycleTests.real_paired_originals

    def select_gate(self):
        path=str(parent.TOOLS/'editor_host_raise.py')
        self.freeze['sources'][path]=parent.read(path)[1]
        dest=Path(self.record['sourceFreezeRef']['path']);dest.write_text(json.dumps(self.freeze))
        self.record['sourceFreezeRef']=parent.read(dest)[1]

    def dependencies(self,calls):
        d=copy.copy(self.d);d.install_client=lambda:None;d.install_ipc=lambda *a:None
        async def blank(bb,out,api):calls.append('blank');return {'HOST_BLANK':True}
        def mouse(original,binding,frame,**kw):calls.append('mouse-api');return SimpleNamespace(original=original,frame=frame)
        async def composed(*a):calls.append('mouse');return {'HOST_ONLY':True}
        d.blank_case=blank;d.mouse_api=mouse;d.composed_case=composed;d.move_request=d.click_request=None
        return d

    def test_selected_gate_runs_in_original_frame_after_blank_before_mouse(self):
        self.select_gate();b=self.bind();calls=[];d=self.dependencies(calls)
        async def raised(binding,blank,original,frame,fd):
            self.assertIs(binding,b);self.assertIs(original.frame,frame);self.assertIs(original.binding,b['blankBinding'])
            self.assertGreaterEqual(fd,3);calls.append('raise');return blank
        with patch.object(gate,'run_same_frame',side_effect=raised):
            self.assertEqual(parent.execute(b,d),{'HOST_ONLY':True})
        self.assertEqual(calls,['blank','raise','mouse-api','mouse'])

    def test_gate_failure_prevents_mouse_and_uses_original_operation_error(self):
        self.select_gate();b=self.bind();calls=[];d=self.dependencies(calls)
        async def refused(*a):calls.append('raise');raise RuntimeError('HOST Raise failure')
        with patch.object(gate,'run_same_frame',side_effect=refused),self.assertRaisesRegex(RuntimeError,'Raise failure'):
            parent.execute(b,d)
        self.assertEqual(calls,['blank','raise']);self.assertEqual(self.events[-1],'operation-error')

    def test_actual_private_guard_holds_gate_failure_then_full_restoration_release(self):
        self.select_gate();binding,guard=self.real_paired_originals('normal')
        b=json.loads(parent.pinned(json.loads(parent.pinned(binding['stoppedBindingRef']))['parentBindingRef']))
        lease=self.d.lease_input(None,None);calls=[];d=self.dependencies(calls)
        @contextmanager
        def operation(root,target,action,**kw):
            with guard.operation(lease,target,action,**kw) as frame:yield frame
        class API:
            def __init__(self,bb,frame):self.binding,self.frame=bb,frame
            def owner(self,owner):
                if owner!=lease['run_id']:raise RuntimeError('HOST foreign owner')
        d.operation=operation;d.original_api=API
        def original_private_guard(root):
            self.assertEqual(root,parent.HARNESS)
            return guard
        d.guard=original_private_guard
        async def refused(*a):calls.append('raise');raise RuntimeError('HOST Raise failure')
        with patch.object(gate,'run_same_frame',side_effect=refused),self.assertRaisesRegex(RuntimeError,'Raise failure'):
            parent.execute(b,d)
        self.assertEqual(guard.status()['phase'],'HOLD');self.assertEqual(calls,['blank','raise'])
        # Fresh stopped fixture evidence adopts actual issued HOLD/nonce, never
        # clears it. Full unchanged public release must archive that obligation.
        sb=json.loads(parent.pinned(binding['stoppedBindingRef']));sb['mode']='failed'
        sb['guardRef']=parent.read(self.h/'runtime/launcher-guard/state.json')[1]
        path=Path(binding['stoppedBindingRef']['path']);path.write_text(json.dumps(sb))
        binding.update(mode='failed',stoppedBindingRef=parent.read(path)[1])
        result=full.execute(binding,self.d)
        self.assertEqual(guard.status()['phase'],'EMPTY');self.assertEqual(result['archivedHolds'],1)
        self.assertEqual(result['restoredRoles'],3);self.assertEqual(result['appDataFiles'],31)
        self.assertEqual(self.roles['canonicalProfile'].read_bytes(),self.original)
        self.assertEqual(parent.pinned(self.b['stagedCandidate']),self.native)


if __name__=='__main__':unittest.main()
