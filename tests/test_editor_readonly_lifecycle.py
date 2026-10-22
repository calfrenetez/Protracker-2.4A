"""Real private source/preparation/copies, synthetic owner/service/FRAME/GUI.

No original target imports, actual locks, service changes or guest operations.
"""
import asyncio
import copy
from contextlib import contextmanager
import json
import os
from pathlib import Path
from types import SimpleNamespace
import unittest
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from unittest.mock import patch

import editor_pointer_parent as parent
import editor_pointer_binding as binder
import editor_readonly_probe as ro
import editor_readonly_capture as blank_case
import editor_mouse_probe_case as mouse_case
import editor_pointer_dispatch as dispatch
import editor_probe_stopped as stopped
import editor_probe_namespace as ns
import editor_capture_namespace as app_ns
import editor_capture_settlement as full
import editor_probe_volume as volume
import test_editor_readonly_probe as fixtures
from test_editor_mouse_probe_case import FakeAPI, picture


class ReadonlyLifecycleTests(unittest.TestCase):
    prepare = fixtures.ReadonlyProbeTests.prepare

    def setUp(self):
        fixtures.ReadonlyFreezeTests.setUp(self)
        # All target observations below are HOST stand-ins, including approval.
        self.events = []; self.phase = 'READY'; self.condition = None; self.released = False
        self.release_calls = self.restore_calls = 0; self.guest_error = False
        self.h = self.root / 'HOST-harness'; (self.h / 'runtime/Dev/Tests').mkdir(parents=True)
        (self.h / 'runtime/launcher-guard').mkdir(); (self.h / 'runtime/test.lock').touch()
        (self.h / 'runtime/launcher-transport.lock').touch(mode=0o600)
        for module in (ns, stopped, app_ns, full):
            p = patch.object(module, 'HARNESS', self.h); p.start(); self.addCleanup(p.stop)
        actual_lock = parent.command_lock
        @contextmanager
        def local_lock(path):
            if str(path) == str(parent.HARNESS / 'runtime/test.lock'):
                path = self.h / 'runtime/test.lock'
            with actual_lock(path) as fd: yield fd
        p = patch.object(parent, 'command_lock', local_lock); p.start(); self.addCleanup(p.stop)
        self.pin_original = parent.pinned
        def pin(ref):
            if ref['path'] == self.active_ref['path']:
                if ref != self.active_ref: raise RuntimeError('HOST wrong active reference')
                return self.active
            try:return self.pin_original(ref)
            except RuntimeError as error:
                actual=parent.read(ref['path'])[1]
                raise RuntimeError('HOST pin refusal '+json.dumps({'expected':ref,'actual':actual},sort_keys=True)) from error
        p = patch.object(parent, 'pinned', pin); p.start(); self.addCleanup(p.stop)
        actual_validate=parent.validate_binding
        p=patch.object(parent,'validate_binding',side_effect=lambda b,**kw:actual_validate(b,pin=pin,**kw))
        p.start();self.addCleanup(p.stop)
        for name in ('NATIVE', 'SOURCE', 'EDITOR'):
            p = patch.object(parent, name, getattr(volume, name)); p.start(); self.addCleanup(p.stop)
        # New runtime output must be absent; only its parent window exists.
        self.out.parent.rmdir(); self.out.parent.parent.rmdir()
        sources = dict(self.plan['sources'])
        ready = volume.save(self.root / 'HOST-start-readiness.json', {
            'state': 'ROOT_ACTUALLY_VERIFIED_HUMAN_APPROVED_DEVBENCH_READY', 'acceptedByRoot': True,
            'authorizationRef': volume.save(self.root / 'HOST-auth.json', {'HOST_ONLY': True}),
            'restartIntentRef': volume.save(self.root / 'HOST-restart-intent.json', {'HOST_ONLY': True}),
            'restartResultRef': volume.save(self.root / 'HOST-restart-result.json', {'HOST_ONLY': True}),
            'observed': {'job': {'pid': '555'}}})
        self.freeze = dict(self.plan, state=ro.FROZEN_STATE, newWindow=str(self.window),
            actualApprovedServiceReadinessRef=ready, sources=sources)
        self.freeze.pop('serviceReadinessRef'); self.freeze.pop('acceptedByRoot')
        freeze_ref = volume.save(self.root / 'SOURCE-FREEZE.json', self.freeze)
        self.b.update(sourceFreezeRef=freeze_ref, sourcePins=sources)
        before = volume.put(self.window / 'before-canonicalProfile.bin', self.original)
        self.owned = {'pid': 999999, 'argv': ['999999', '999999', 'HOST_VM'], 'socket_inode': 123}
        self.service = {'pid': 555, 'argv': ['555', '555', 'HOST_SERVICE']}
        geometry = volume.save(self.root / 'HOST-native-proof.json', {
            'state': 'ARCHIVED_ACTUAL_BUILD_GEOMETRY_SOURCE_REVIEW_ONLY', 'nativeCellCenter': [218,424],
            'expectedRow':4, 'expectedChannel':1, 'expectedField':0,
            'clickDispatchUsesUnchangedIntuiMessageCoordinates':True,
            'candidate':json.loads(parent.pinned(self.refs['capturePreparation']))['originalCandidate'],
            'nativeSourceCopies':{}})
        self.record = {'state':binder.READONLY_BIND, 'acceptedByRoot':True,
            'allowedActions': {'guestCPUAdmissions':1, 'bindingWrites':2, 'applicationLaunches':0,
                'input':0, 'serviceChanges':0, 'guardReleases':0}, 'limits':{'whole':60},
            'windowDirectory':str(self.window), 'sourceFreezeRef':freeze_ref,
            'readonlyPreparationRef':self.b['readonlyPreparationRef'],
            'reservationRef':volume.save(self.window / 'reserve-result.json', {
                'state':'FRESH_OWN_RESERVATION_NO_LAUNCH_OR_INPUT', 'run_id':'fixture-owner',
                'window':str(self.window), 'target':'amiberry-030','serviceReadinessRef':ready}),
            'startupResultRef':volume.save(self.window / 'startup-result.json', {'returncode':0}),
            'ownedProcessRef':volume.save(self.window / 'ROOT_CURRENT_STARTED_PROCESS.json', self.owned),
            'serviceProcessRef':volume.save(self.window / 'ROOT_CURRENT_DEVBENCH_PROCESS.json', self.service),
            'nativeViewRef':volume.save(self.window / 'ROOT_NATIVE_WORKBENCH_VIEW.json', {
                'actuallyViewed':True,'pid':999999,'applicationVisible':False}),
            'originalProfileRef':before, 'activeProfileRef':self.active_ref,
            'preservationBeforeRef':volume.save(self.window / 'fresh-preservation-before.json', {'HOST_ONLY':True}),
            'preparationRef':self.refs['capturePreparation'], 'geometryProofRef':geometry,
            'probeNativeRef':self.refs['nativeCandidate'], 'probeSourceRef':self.refs['nativeSource'],
            'probeCompileRef':self.refs['compileReceipt'], 'runLabel':self.label}
        outer = self
        class Guard:
            def __init__(self, root): pass
            def status(self):
                return {'phase':'EMPTY' if outer.released else outer.phase,
                    'reservation':None if outer.released else {'lease':{'run_id':'fixture-owner'}},
                    'holds':[{}] if outer.phase=='HOLD' and not outer.released else [],
                    'inflight':None, 'fence':7}
            @contextmanager
            def _locked(self): yield None, {}
            def _owner(self, state, lease, target):
                if lease['run_id']!='fixture-owner': raise RuntimeError('HOST owner differs')
            def release(self, lease, proof):
                outer.release_calls += 1
                cleanup=json.loads((outer.window/'cleanup-release-fact.json').read_text())
                retention=cleanup['readonlyProbeInputRetention']['facts']['readonlyProbeInput']
                outer.assertEqual(retention['facts']['probeFilesRemoved'],0)
                outer.assertEqual(parent.pinned(outer.b['stagedCandidate']),outer.native)
                outer.assertFalse(outer.run.exists())
                outer.assertEqual(set(proof['records']),{'authorization','restoration','absence','cleanup'})
                outer.released=True
                history=outer.h/'runtime/launcher-guard/history';history.mkdir()
                (history/'HOST-archive.json').write_text(json.dumps({'proof':proof,
                    'reservation':{'lease':lease},'holds':[{}] if outer.phase=='HOLD' else []}))
                return {'released':True,'archive':'HOST-archive.json','fence':8}
        class Host:
            def command(self, argv, expected):
                if '-p' in argv:return '555 555 OTHER' if outer.condition=='service' else '555 555 HOST_SERVICE'
                if argv[0].endswith('pgrep'):return '999999' if outer.condition=='vm' else ''
                if argv[0].endswith('lsof'):return 'listener' if outer.condition=='port' else ''
                return '42 999999' if outer.condition=='group' else '42 42'
            def socket_absent(self):return outer.condition!='socket'
            def health(self):return {'host':'127.0.0.1','port':2345,'connected':outer.condition=='bridge'}
        @contextmanager
        def operation(*args, **kw):
            self.events.append('operation-enter')
            try: yield {'HOST_FRAME':True}
            except BaseException: self.events.append('operation-error'); raise
            else: self.events.append('operation-return')
        class API:
            pinned=staticmethod(pin)
            def __init__(self, b, frame):self.binding=b;self.frame=frame
            def owner(self, owner):
                if owner!='fixture-owner':raise RuntimeError('HOST API owner loss')
            def clock(self):return 100.
        def guest(*args):
            self.events.append('guest')
            if self.guest_error:raise RuntimeError('HOST guest failure')
        self.d=SimpleNamespace(guard=Guard, lease_input=lambda *a:{'run_id':'fixture-owner'},
            operation=operation, original_api=API, guest=guest, blank_scope=blank_case,
            extras={'service_readiness':SimpleNamespace(validate_records=lambda *a:None),
                    'fresh_preservation':SimpleNamespace(verify_current=lambda before:{'HOST_ONLY':True})},
            observations=Host(), decode=json.loads)

    def bind(self):
        ref=binder.execute_readonly(self.record,self.d)
        return json.loads(parent.pinned(ref))

    def test_distinct_start_binds_paired_blank_and_readonly_child_without_dev_staging(self):
        b=self.bind()
        self.assertEqual(b['state'],ro.PARENT_STATE)
        self.assertEqual(b['blankBinding']['state'],ro.BLANK_STATE)
        self.assertEqual(b['blankBinding']['readonlyProbe'],b['passiveProbe'])
        self.assertNotIn('namespaceStageRef',b['passiveProbe'])
        self.assertEqual(b['passiveProbe']['guestCandidate'],'PTProbe:PTEditorPointerV2')
        self.assertEqual(self.events,['operation-enter','guest','operation-return'])
        parent.validate_binding(b,readonly=True)
        with self.assertRaises(RuntimeError):parent.validate_binding(b)
        with self.assertRaises(RuntimeError):binder.execute_readonly(self.record,self.d)
        self.assertEqual(self.events.count('guest'),1)

    def test_legacy_binder_cannot_consume_new_start_scope(self):
        with self.assertRaises(RuntimeError):binder.execute(self.record,self.d)
        self.assertFalse(self.events)

    def test_changed_active_profile_or_service_refuses_before_cpu_admission(self):
        self.active=self.original
        with self.assertRaises(RuntimeError):self.bind()
        self.assertFalse(self.events)
        self.active=self.original+volume.verify(self.b['readonlyPreparationRef'])['appendASCII'].encode()
        self.condition='service'
        with self.assertRaises(RuntimeError):self.bind()
        self.assertNotIn('guest',self.events)
        self.assertTrue((self.window/'mounted-guest-admission-intent.json').exists())
        self.assertFalse((self.window/'mouse-parent-binding.json').exists())

    def test_wrong_preparation_or_unreviewed_template_refuses_before_intent(self):
        self.record['readonlyPreparationRef']=self.refs['capturePreparation']
        with self.assertRaises(RuntimeError):self.bind()
        self.assertFalse(self.events)
        self.record['readonlyPreparationRef']=self.b['readonlyPreparationRef']
        private=self.root/'capture_case_mounted.py';private.write_bytes(b'# legacy or altered blank\n')
        # Even a new pin cannot substitute a different private blank source.
        f=copy.deepcopy(self.freeze);f['sources'][str(private)]=parent.read(private)[1]
        Path(self.record['sourceFreezeRef']['path']).write_text(json.dumps(f))
        self.record['sourceFreezeRef']=parent.read(self.record['sourceFreezeRef']['path'])[1]
        with self.assertRaises(RuntimeError):self.bind()
        self.assertFalse(self.events)

    def test_guest_failure_keeps_intent_without_bindings_and_cannot_replay(self):
        self.guest_error=True
        with self.assertRaisesRegex(RuntimeError,'HOST guest failure'):self.bind()
        with self.assertRaisesRegex(RuntimeError,'Never replay'):self.bind()
        self.assertEqual(self.events.count('guest'),1)
        self.assertFalse((self.window/'capture-binding.json').exists())

    def test_readonly_dispatch_admits_zero_probe_stages_and_retains_original_budgets(self):
        b=self.bind();ref=parent.read(self.window/'mouse-parent-binding.json')[1]
        actual,freeze,window=dispatch.validate(ref)
        self.assertEqual(actual,b);self.assertEqual(actual['limits']['whole'],240)
        self.assertNotIn('probeStage',actual['limits'])
        self.assertEqual(actual['blankBinding']['limits']['whole'],300)

    def test_new_parent_uses_same_frame_and_no_namespace_stage(self):
        b=self.bind();frames=[];calls=[]
        async def capture(bb,out,api):
            self.assertIs(api.binding,bb);frames.append(api.frame);calls.append('blank');return {'HOST_BLANK':True}
        def mouse(original,binding,frame,**kw):
            self.assertIs(frame,original.frame);frames.append(frame);return SimpleNamespace(original=original,frame=frame,binding=binding)
        async def composed(binding,captured,out,mouse_api,original):
            self.assertIs(mouse_api.original,original);calls.append('mouse');return {'HOST_ONLY':True}
        d=copy.copy(self.d);d.install_client=lambda:None;d.install_ipc=lambda *a:None
        d.blank_case=capture;d.mouse_api=mouse;d.composed_case=composed;d.move_request=d.click_request=None
        d.namespace_stage=lambda *a,**kw:self.fail('Read-only parent attempted Dev staging')
        self.assertEqual(parent.execute(b,d),{'HOST_ONLY':True})
        self.assertEqual(calls,['blank','mouse']);self.assertIs(frames[0],frames[1])

    def test_mouse_checks_paired_input_before_every_original_gui_boundary(self):
        for name in ('state_query','capture','move','click','root_observation'):
            with self.subTest(boundary=name):
                events=[]
                async def action(*a):events.append(name);return name
                mouse=SimpleNamespace(**{name:action})
                probe=SimpleNamespace(binding=self.b,original=self.api)
                wrapper=mouse_case.ReadonlyMouseProbeAPI(mouse,probe)
                self.active=self.original
                with self.assertRaises(RuntimeError):asyncio.run(getattr(wrapper,name)())
                self.assertEqual(events,[])
                self.active=self.original+volume.verify(self.b['readonlyPreparationRef'])['appendASCII'].encode()
                self.assertEqual(asyncio.run(getattr(wrapper,name)()),name)
                self.assertEqual(events,[name])

    def test_mouse_input_drift_after_move_prevents_probe_and_click(self):
        b=self.bind();(self.window/self.label).mkdir();api=FakeAPI();api.original=self.api;api.frame=object()
        original=SimpleNamespace(binding=b['blankBinding'],frame=api.frame,owner=self.api.owner,pinned=self.api.pinned)
        api.original=original;api.binding=b
        api.baseline=api.baseline.replace(b'blank-capture-firstmouse-r1',self.label.encode())
        self.api.binding=b['blankBinding']; self.api.frame=api.frame
        wrapped_probe=ro.ReadonlyEditorPointerProbe(b['passiveProbe'],self.api,io=self.io)
        wrapper=mouse_case.ReadonlyMouseProbeAPI(api,wrapped_probe)
        move=api.move
        async def changed(*args):
            value=await move(*args);self.active=self.original;return value
        api.move=changed
        with self.assertRaises(RuntimeError):
            asyncio.run(mouse_case.run_selection(b,{
                'state':'BLANK_LOG_AND_REGISTERED_CAPTURE_RECORDED','ownerRunID':'fixture-owner',
                'attempts':{'candidate-readback':1,'environment-setup':1,'application-launch':1,'capture':1},
                'capture':picture(1)},self.window/self.label/'mouse-phase',wrapper,readonly=True))
        self.assertIn('move',api.events);self.assertNotIn('click',api.events)
        self.assertEqual(self.events.count(('execute','PTProbe:PTEditorPointerV2')),0)


    def test_parent_rechecks_complete_profile_before_any_original_operation(self):
        b=self.bind();self.active=self.original
        with self.assertRaisesRegex(RuntimeError,'Complete actual paired profile'):
            parent.validate_binding(b,readonly=True)

    def test_new_mouse_adapter_requires_same_original_api_frame_and_binding(self):
        b=self.bind();frame=object();self.api.binding=b['blankBinding'];self.api.frame=frame
        mouse=SimpleNamespace(original=self.api,frame=object(),binding=b)
        with self.assertRaisesRegex(RuntimeError,'Same actual original read-only API/FRAME'):
            asyncio.run(mouse_case.run_readonly_editor_mouse_probe(b,{},self.window/'none',mouse,self.api))
        mouse.frame=frame
        async def selection(binding,blank,out,api,**kw):
            self.assertTrue(kw['readonly']);self.assertIsInstance(api.probe,ro.ReadonlyEditorPointerProbe)
            self.assertIs(api.probe.original,self.api);return {'HOST_ONLY':True}
        with patch.object(mouse_case,'run_selection',side_effect=selection):
            self.assertEqual(asyncio.run(mouse_case.run_readonly_editor_mouse_probe(b,{},self.window/'none',mouse,self.api)),{'HOST_ONLY':True})

    def blank_api(self, binding, *, drift=None):
        from editor_blank_capture_protocol import recent_line, BLANK_FRAME
        self.blank_events=[];outer=self;out=Path(binding['outputDirectory'])
        editor=parent.pinned(binding['candidate'])
        for name,value in (('CANDIDATE_BYTES',len(editor)),('CANDIDATE_SHA',parent.digest(editor)[1])):
            p=patch.object(blank_case,name,value);p.start();self.addCleanup(p.stop)
        class Guest:
            def __init__(self,harness,evidence):
                outer.blank_events.append('Guest-construction')
                self.share=outer.h/'runtime/Dev/Tests';self.device='Dev:Tests/'
                self.launch=self.share/('launch-'+out.name);self.count=0
            def start(self):
                self.count+=1;outer.blank_events.append('start-'+str(self.count));run=self.share/out.name
                if self.count==1:
                    settings,_=blank_case.environment_script(self.device+out.name)
                    for i,value in enumerate(settings.values()):(run/('active-'+str(i))).write_bytes(value.encode())
                    for leaf in ('backup-ready','setup-done'):(run/leaf).write_bytes(b'ready\n')
                    (out/'launch.log').write_bytes(b'HOST environment setup')
                else:
                    (run/'editor.log').write_bytes(recent_line(out.name)+b'\n'
                        b'EDITOR READY channels=4 patterns=1 bitmap_bytes=163840 history_bytes=182388\n'+BLANK_FRAME+b'\n')
        GuestType=Guest
        class API:
            Guest=GuestType
            pinned=staticmethod(outer.api.pinned);read=staticmethod(parent.read)
            def owner(self,owner):outer.api.owner(owner)
            def clock(self):return 100.
            async def sleep(self,n):pass
            def save(self,path,value):volume.save(path,value)
            def put(self,path,body):volume.put(path,body)
            def replace(self,path,before,after):
                outer.assertEqual(Path(path).read_bytes(),before);Path(path).write_bytes(after)
            def prepare_run(self,guest,evidence):
                run=guest.share/out.name;run.mkdir();return run
            def require_running_guest(self,*a):self.owner('fixture-owner')
            def optional(self,path,maximum):return parent.read(path,maximum)[0] if path.exists() else None
            def put_unique_snapshot(self,evidence,body):volume.put(evidence/'HOST-observed-log.log',body)
            async def readback(self,guest,path):
                outer.blank_events.append('editor-readback');path.write_bytes(editor)
                if drift=='after-readback':outer.active=outer.original
            async def capture(self,path):outer.blank_events.append('capture');return picture(1)
        return API()

    def test_paired_blank_preserves_original_one_readback_setup_launch_and_capture(self):
        b=self.bind();api=self.blank_api(b['blankBinding'])
        result=asyncio.run(blank_case.run_case(b['blankBinding'],Path(b['outputDirectory']),api))
        self.assertEqual(result['state'],'BLANK_LOG_AND_REGISTERED_CAPTURE_RECORDED')
        self.assertEqual(result['attempts'],{'candidate-readback':1,'environment-setup':1,'application-launch':1,'capture':1})
        self.assertEqual(self.blank_events,['Guest-construction','editor-readback','start-1','start-2','capture'])
        self.assertFalse(result['wholeApplicationAcceptance']);self.assertFalse(result['cleanExitClaimed'])

    def test_blank_profile_drift_after_readback_keeps_failure_without_any_launch(self):
        b=self.bind();api=self.blank_api(b['blankBinding'],drift='after-readback')
        with self.assertRaises(RuntimeError):asyncio.run(blank_case.run_case(b['blankBinding'],Path(b['outputDirectory']),api))
        result=json.loads((Path(b['outputDirectory'])/'result.json').read_text())
        self.assertEqual(result['attempts'],{'candidate-readback':1})
        self.assertEqual(self.blank_events,['Guest-construction','editor-readback'])
        with self.assertRaises(RuntimeError):asyncio.run(blank_case.run_case(b['blankBinding'],Path(b['outputDirectory']),api))
        self.assertEqual(self.blank_events,['Guest-construction','editor-readback'])

    def test_blank_foreign_child_cannot_construct_guest(self):
        b=self.bind();api=self.blank_api(b['blankBinding'])
        b['blankBinding']['ownerRunID']='OTHER'
        with self.assertRaises(RuntimeError):asyncio.run(blank_case.run_case(b['blankBinding'],Path(b['outputDirectory']),api))
        self.assertEqual(self.blank_events,[])

    def full_binding(self, mode='normal'):
        b=self.bind(); self.phase='HOLD' if mode=='failed' else 'READY'
        sb={'state':stopped.READONLY_STATE,'acceptedByRoot':True,'target':'amiberry-030',
            'mode':mode,'limits':{'whole':60},'allowedActions':dict(stopped.READONLY_ALLOWED),
            'ownerRunID':'fixture-owner','windowDirectory':str(self.window),
            'parentBindingRef':parent.read(self.window/'mouse-parent-binding.json')[1],
            'guardRef':volume.put(self.h/'runtime/launcher-guard/state.json',b'HOST_GUARD'),
            'stopReceiptRef':volume.save(self.window/'ROOT_ACTUAL_OWNED_STOP.json',{
                'ownerRunID':'fixture-owner','pid':999999,'processAndGroupAbsent':True}),
            'ownedProcessRef':self.record['ownedProcessRef'],'serviceProcessRef':self.record['serviceProcessRef']}
        self.run=self.h/'runtime/Dev/Tests'/self.label;self.run.mkdir()
        (self.run/'copies').mkdir();(self.run/'editor.log').write_bytes(b'HOST failed/successful application evidence')
        (self.run.parent/('launch-'+self.label)).write_bytes(b'HOST original launcher')
        self.app=self.root/'app-data';self.app.mkdir()
        p=patch.object(full,'APP_ROOT',self.app);p.start();self.addCleanup(p.stop)
        rows={}
        for name in sorted(full.THEMES)+['Data/file%02d'%i for i in range(29)]:
            path=self.app/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(name.encode())
            backup=self.root/'saved-app-data'/name;backup.parent.mkdir(parents=True,exist_ok=True)
            backup.write_bytes(path.read_bytes())
            rows[name]={'original':parent.read(path)[1],'backup':str(backup.relative_to(self.root))}
        self.rows=rows;self.roles={};scope={}
        for name in ('canonicalProfile','emulatorState','bridgeEnabled'):
            path=self.h/'runtime'/name;path.write_bytes(('HOST ORIGINAL '+name).encode())
            self.roles[name]=path;scope[name]=parent.read(path)[1]
            before=self.window/('before-'+name+'.bin')
            if name!='canonicalProfile':before.write_bytes(path.read_bytes())
            else:
                # Canonical beforeimage was already pinned by the actual binder.
                path.write_bytes(self.original);scope[name]=parent.read(path)[1]
            path.write_bytes(b'HOST ACTIVE SESSION')
        self.full_b={'state':full.READONLY_STATE,'acceptedByRoot':True,'ownerRunID':'fixture-owner',
            'windowDirectory':str(self.window),'mode':mode,'allowedActions':dict(full.ALLOWED),'limits':{'whole':180},
            'stoppedBindingRef':volume.save(self.window/'probe-stopped-binding.json',sb),
            'appDataBeforeRef':volume.save(self.window/'current-app-data-before-and-backups.json',{'files':rows}),
            'roleBaselineRef':volume.save(self.window/'before-owned-lifecycle-files.json',scope),
            'preservationBeforeRef':self.record['preservationBeforeRef']}
        for name in full.THEMES:
            path=self.app/name;os.utime(path,ns=(path.stat().st_atime_ns,rows[name]['original']['fullFacts']['st_mtime_ns']+1000000))
        @contextmanager
        def locks():
            with parent.command_lock(self.h/'runtime/test.lock'),parent.command_lock(self.h/'runtime/launcher-transport.lock'):
                yield
        def restore(a,ap,window,action):
            self.restore_calls+=1;restored={}
            with locks():
                for role,path in self.roles.items():
                    path.write_bytes((self.window/('before-'+role+'.bin')).read_bytes())
                    os.utime(path,ns=(path.stat().st_atime_ns,scope[role]['fullFacts']['st_mtime_ns']))
                    restored[role]=parent.read(path)[1]
                self.active=self.original  # Simulated canonical paired-profile restoration.
                path=self.window/'restore-canonical-profile-session-result.json'
                volume.save(path,{'state':'THREE_HOST_FILE_ORIGINAL_BODIES_RESTORED_AFTER_VERIFIED_OWNED_STOP',
                    'restoredFull4s':restored})
                return path
        self.d.files=SimpleNamespace(ROLES=self.roles,locks=locks,resident=lambda path,private=False:parent.read(path),
            pinned=lambda ref,private=False:parent.pinned(ref),
            admissions=lambda path,action:(json.loads(parent.read(path)[0]),parent.read(path)[1],self.window),execute=restore)
        self.d.preservation=SimpleNamespace(verify_current=lambda before:{'HOST_PROTECTED':True})
        return self.full_b

    def test_full_normal_retains_readonly_input_restores_all_roles_and_releases_once(self):
        binding=self.full_binding();result=full.execute(binding,self.d)
        self.assertEqual(result['state'],'FULL_STOPPED_EDITOR_AND_PROBE_RESTORED_AND_RELEASED')
        self.assertEqual((self.restore_calls,self.release_calls),(1,1))
        self.assertEqual(result['appDataFiles'],31)
        self.assertEqual(parent.pinned(self.b['stagedCandidate']),self.native)
        self.assertEqual(self.active,self.original)
        self.assertFalse(result['runtimeAcceptance']);self.assertFalse(result['nativeCleanExitClaimed'])
        with self.assertRaises(RuntimeError):full.execute(binding,self.d)
        self.assertEqual(self.release_calls,1)

    def test_full_failed_mode_keeps_hold_until_original_release_and_retains_probe(self):
        binding=self.full_binding('failed');result=full.execute(binding,self.d)
        self.assertEqual(result['archivedHolds'],1);self.assertEqual(self.release_calls,1)
        self.assertEqual([p.name for p in self.destination.iterdir()],['PTEditorPointerV2'])
        saved=self.window/'preserved-readonly-probe-input'
        self.assertEqual((saved/'PTEditorPointerV2').read_bytes(),self.native)

    def test_changed_stopped_owner_presence_or_service_prevents_input_preservation(self):
        binding=self.full_binding('failed')
        for condition in ('service','vm','group','socket','port','bridge'):
            with self.subTest(condition=condition):
                self.condition=condition
                with self.assertRaises(RuntimeError):full.execute(binding,self.d)
                self.assertFalse((self.window/'preserved-readonly-probe-input').exists())
                self.assertTrue(self.run.exists());self.assertEqual(self.restore_calls,0)
        self.assertEqual(self.release_calls,0)

    def test_changed_retained_probe_never_reaches_restoration_or_release(self):
        binding=self.full_binding()
        Path(self.b['stagedCandidate']['path']).chmod(0o700)
        with self.assertRaises(RuntimeError):full.execute(binding,self.d)
        self.assertEqual((self.restore_calls,self.release_calls),(0,0));self.assertTrue(self.run.exists())

    def test_retention_drift_after_restoration_never_enters_release_proof(self):
        binding=self.full_binding()
        def drift(before):
            path=self.window/'preserved-readonly-probe-input/PTEditorPointerV2'
            path.write_bytes(b'HOST changed preserved evidence')
            return {'HOST_ONLY':True}
        self.d.preservation.verify_current=drift
        with self.assertRaises(RuntimeError):full.execute(binding,self.d)
        self.assertEqual((self.restore_calls,self.release_calls),(1,0))
        self.assertFalse((self.window/'release-proof.json').exists())
        self.assertEqual(parent.pinned(self.b['stagedCandidate']),self.native)

    def test_full_scope_cannot_substitute_legacy_probe_cleanup(self):
        binding=self.full_binding();binding['state']=full.STATE
        with self.assertRaises(RuntimeError):full.execute(binding,self.d)
        self.assertEqual((self.restore_calls,self.release_calls),(0,0));self.assertTrue(self.run.exists())

    def real_paired_originals(self, mode):
        """Unchanged public guard and file helper; only private fixture endpoints."""
        import importlib.util,types
        binding=self.full_binding(mode)
        helper=self.root/'canonical_profile_session_files.py';body=helper.read_bytes()
        self.assertEqual(parent.digest(body),binder.SAVED_FILE_HELPER)
        f=types.ModuleType('HOST_readonly_original_files');f.__file__=str(helper)
        exec(compile(body,str(helper),'exec'),f.__dict__)
        p=json.loads(parent.pinned(json.loads(parent.pinned(binding['stoppedBindingRef']))['parentBindingRef']))
        binder.configure_readonly_probe_saved_files(f,p['sourcePins'],p['blankBinding']['localVolumePreparation'],
            p['passiveProbe']['readonlyPreparationRef'],p['blankBinding']['originalProfileBeforeimage'])
        combined=volume.verify(self.b['readonlyPreparationRef'])['appendASCII'].encode()
        self.assertEqual(f.APPEND,combined)
        self.assertIn(b'filesystem2=ro,DH4:PTProbe:',f.APPEND)
        f.I=self.h;f.ROLES=self.roles;f.ORIGINAL=parent.digest(self.original)
        f.SESSION=parent.digest(self.original+combined)
        self.roles['canonicalProfile'].write_bytes(self.original+combined)
        role_scope=json.loads(parent.pinned(binding['roleBaselineRef']))
        for role in self.roles:
            backup=self.window/('before-'+role+'.bin')
            if backup.stat().st_mode & 0o777 != 0o600:backup.chmod(0o600)
            role_scope[role]['absent']=False
        path=Path(binding['roleBaselineRef']['path']);path.write_text(json.dumps(role_scope))
        binding['roleBaselineRef']=parent.read(path)[1]
        volume.save(self.window/'patch-canonical-profile-session-result.json',{
            'state':'CANONICAL_PROFILE_SESSION_APPEND_APPLIED_ORIGINAL_BYTES_PRESERVED'})
        installed=parent.HARNESS/'scripts/launcher_guard.py'
        spec=importlib.util.spec_from_file_location('HOST_readonly_original_guard',installed)
        original=importlib.util.module_from_spec(spec);spec.loader.exec_module(original)
        state=self.h/'runtime/launcher-guard';(state/'state.json').unlink();state.rmdir()
        guard=original.initialize(self.h,[],{'mode':'temporary-fixture','approved':True,
            'legacy_holds_verified':True,'record_sha256':'1'*64})
        lease=guard.reserve('amiberry-030','HOST only','Private paired-profile restoration fixture')
        if mode=='failed':guard.hold(lease,'HOST synthetic failure; no target')
        owner=lease['run_id'];p['ownerRunID']=owner;p['blankBinding']['ownerRunID']=owner
        p['passiveProbe']['ownerRunID']=owner;p['blankBinding']['readonlyProbe']['ownerRunID']=owner
        path=self.window/'mouse-parent-binding.json';path.write_text(json.dumps(p))
        sb=json.loads(parent.pinned(binding['stoppedBindingRef']));sb.update(ownerRunID=owner,
            parentBindingRef=parent.read(path)[1],guardRef=parent.read(state/'state.json')[1])
        path=self.window/'ROOT_ACTUAL_OWNED_STOP.json';path.write_text(json.dumps({
            'ownerRunID':owner,'pid':999999,'processAndGroupAbsent':True}))
        sb['stopReceiptRef']=parent.read(path)[1]
        path=Path(binding['stoppedBindingRef']['path']);path.write_text(json.dumps(sb))
        binding.update(ownerRunID=owner,stoppedBindingRef=parent.read(path)[1])
        self.d.guard=original.LauncherGuard;self.d.lease_input=lambda *a:lease
        self.d.decode=original._decode;self.d.files=f
        return binding,guard

    def test_original_guard_and_full_paired_profile_restoration_in_normal_private_fixture(self):
        binding,guard=self.real_paired_originals('normal');result=full.execute(binding,self.d)
        self.assertEqual(guard.status()['phase'],'EMPTY');self.assertEqual(result['archivedHolds'],0)
        self.assertEqual(parent.pinned(self.b['stagedCandidate']),self.native)
        self.assertEqual(self.roles['canonicalProfile'].read_bytes(),self.original)
        self.assertEqual(result['restoredRoles'],3);self.assertEqual(result['appDataFiles'],31)

    def test_original_guard_and_full_paired_profile_restoration_in_failed_private_fixture(self):
        binding,guard=self.real_paired_originals('failed');result=full.execute(binding,self.d)
        self.assertEqual(guard.status()['phase'],'EMPTY');self.assertEqual(result['archivedHolds'],1)
        self.assertEqual(parent.pinned(self.b['stagedCandidate']),self.native)
        self.assertEqual(self.roles['canonicalProfile'].read_bytes(),self.original)


if __name__=='__main__':unittest.main()
