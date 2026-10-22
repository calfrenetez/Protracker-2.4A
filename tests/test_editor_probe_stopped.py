"""Stopped entrypoint integration in private HOST fixtures and fake original APIs."""
from contextlib import contextmanager
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import editor_pointer_parent as parent
import editor_probe_namespace as ns
import editor_probe_stopped as stopped


class StoppedProbeTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='pt24g-stopped-HOST-',dir='/private/tmp')
        self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name);self.h=self.root/'harness'
        for module in (parent,ns,stopped):
            p=patch.object(module,'HARNESS',self.h);p.start();self.addCleanup(p.stop)
        (self.h/'runtime/Dev/Tests').mkdir(parents=True)
        (self.h/'runtime/test.lock').touch();(self.h/'runtime/launcher-guard').mkdir()
        self.window=self.root/'window';self.window.mkdir()
        self.owner='HOST_FIXTURE_OWNER';native=b'HOST native';source=b'HOST source'
        for name,body in [('NATIVE',native),('SOURCE',source)]:
            p=patch.object(ns,name,ns.digest(body));p.start();self.addCleanup(p.stop)
        sources={str(Path(stopped.__file__)):parent.read(stopped.__file__)[1]}
        for name in ('launcher_guard','launcher_access'):
            path=self.h/'scripts'/(name+'.py');path.parent.mkdir(exist_ok=True)
            sources[str(path)]=ns.put(path,b'FAKE_HOST_MODULE_NEVER_IMPORTED')
        compile_receipt={'state':'PASSIVE_EDITOR_V2_SOURCE_AND_NATIVE_COMPILE_HOST_CHECKED_NOT_EXECUTED',
            'binary':{'bytes':len(native),'sha256':ns.digest(native)[1]},
            'sourcePinsEqualBeforeAfter':{'tools/diagnostics/native_editor_pointer_snapshot.c':
                {'bytes':len(source),'sha256':ns.digest(source)[1]}}}
        probe={'ownerRunID':self.owner,'windowDirectory':str(self.window),'sourcePins':sources,
               'nativeCandidate':ns.put(self.root/'native',native),'nativeSource':ns.put(self.root/'source.c',source),
               'compileReceipt':ns.save(self.root/'compile.json',compile_receipt)}
        with parent.command_lock(self.h/'runtime/test.lock') as fd:
            stage=ns.stage(probe,self.window/'passive-probe-stage',
                SimpleNamespace(stage_owner=lambda owner:self.assertEqual(owner,self.owner)),lock_fd=fd)
        probe['namespaceStageRef']=stage
        owned={'pid':999999,'argv':['999999','999999','FAKE_VM'],'socket_inode':123}
        service={'pid':555,'argv':['555','555','python FAKE_SERVICE']}
        freeze=ns.save(self.root/'freeze.json',{'state':parent.FROZEN_STATE,'sources':sources})
        self.parent={'state':'ROOT_ADMITTED_EDITOR_MOUSE_WITH_PASSIVE_V2','ownerRunID':self.owner,
            'passiveProbe':probe,'sourceFreezeRef':freeze,'sourcePins':sources,
            'blankBinding':{'ownedProcess':owned,'modulePaths':{name:str(self.h/'scripts'/(name+'.py'))
                for name in ('launcher_guard','launcher_access')}}}
        self.b={'state':stopped.STATE,'acceptedByRoot':True,'target':'amiberry-030','mode':'normal',
            'ownerRunID':self.owner,'windowDirectory':str(self.window),'limits':{'whole':60},
            'allowedActions':{'probeFilesRemoved':1,'probeDirectoriesRemoved':1,'targetCommands':0,
                             'signals':0,'serviceChanges':0,'guardReleases':0},
            'parentBindingRef':ns.save(self.window/'mouse-parent-staged-binding.json',self.parent),
            'guardRef':ns.put(self.h/'runtime/launcher-guard/state.json',b'FAKE_HOST_GUARD'),
            'ownedProcessRef':ns.save(self.window/'ROOT_CURRENT_STARTED_PROCESS.json',owned),
            'serviceProcessRef':ns.save(self.window/'ROOT_CURRENT_DEVBENCH_PROCESS.json',service),
            'stopReceiptRef':ns.save(self.window/'ROOT_ACTUAL_OWNED_STOP.json',
                {'ownerRunID':self.owner,'pid':999999,'processAndGroupAbsent':True})}
        self.phase='READY';self.lease_owner=self.owner;self.condition=None;self.events=[]
        outer=self
        class Guard:
            def __init__(self,root):outer.events.append('guard')
            def status(self):return {'phase':outer.phase,'holds':[{}] if outer.phase=='HOLD' else [],
                'inflight':{} if outer.condition=='inflight' else None,'fence':7}
            @contextmanager
            def _locked(self):yield None,{}
            def _owner(self,state,lease,target):
                if lease['run_id']!=outer.owner:raise RuntimeError('FAKE original owner refusal')
        class Host:
            def command(self,argv,expected):
                outer.events.append(tuple(argv))
                if '-p' in argv:return '555 555 wrong service' if outer.condition=='service' else '555 555 python FAKE_SERVICE'
                if argv[0].endswith('pgrep'):return '999999' if outer.condition=='vm' else ''
                if argv[0].endswith('lsof'):return 'listener' if outer.condition=='port' else ''
                return '42 999999' if outer.condition=='group' else '42 42'
            def socket_absent(self):return outer.condition!='socket'
            def health(self):return {'host':'127.0.0.1','port':2345,'connected':outer.condition=='bridge'}
        self.d=SimpleNamespace(guard=Guard,lease_input=lambda *a:{'run_id':self.lease_owner},observations=Host())
        self.directory=ns.paths(self.owner)[1]

    def run_cleanup(self):return stopped.execute(self.b,self.d)

    def test_normal_entrypoint_preserves_probe_and_records_fact_without_release(self):
        fact=self.run_cleanup();value=json.loads(ns.pinned(fact))
        self.assertTrue(value['probePreservedAndAbsent']);self.assertFalse(value['guardReleaseClaimed'])
        self.assertFalse(self.directory.exists())
        observed=json.loads((self.window/'probe-stopped-observations.json').read_text())
        self.assertGreaterEqual(len(observed['absenceObservations']),8)
        self.assertEqual(observed['targetOperations'],0)
        supplement=stopped.release_cleanup_supplement(self.b,fact,self.d)
        self.assertEqual(supplement['probeNamespaceCleanup']['factRef'],fact)

    def test_failed_entrypoint_retains_hold_and_uses_failed_mode(self):
        self.phase='HOLD';self.b['mode']='failed'
        fact=self.run_cleanup();self.assertEqual(json.loads(ns.pinned(fact))['mode'],'failed')
        self.assertEqual(self.phase,'HOLD')
        supplement=stopped.release_cleanup_supplement(self.b,fact,self.d)
        self.assertEqual(supplement['probeNamespaceCleanup']['facts']['mode'],'failed')

    def test_changed_cleanup_fact_is_not_adopted_by_release_record(self):
        fact=self.run_cleanup();path=Path(fact['path']);value=json.loads(ns.pinned(fact))
        value['probePreservedAndAbsent']=False;path.write_text(json.dumps(value))
        with self.assertRaises(RuntimeError):stopped.release_cleanup_supplement(self.b,parent.read(path)[1],self.d)

    def test_live_service_process_group_socket_port_and_bridge_refusals_preserve_probe(self):
        for condition in ('service','vm','group','socket','port','bridge','inflight'):
            self.condition=condition
            with self.subTest(condition=condition),self.assertRaises(RuntimeError):self.run_cleanup()
            self.assertTrue((self.directory/'PTEditorPointerV2').is_file())
            self.assertFalse((self.window/'probe-stopped-intent.json').exists())

    def test_wrong_current_lease_or_guard_mode_never_removes_probe(self):
        self.lease_owner='OTHER'
        with self.assertRaises(RuntimeError):self.run_cleanup()
        self.lease_owner=self.owner;self.phase='HOLD'
        with self.assertRaises(RuntimeError):self.run_cleanup()
        self.assertTrue(self.directory.exists())

    def test_guard_or_stop_file_drift_refuses_before_dependencies(self):
        Path(self.b['guardRef']['path']).write_bytes(b'changed')
        with self.assertRaises(RuntimeError):self.run_cleanup()
        self.assertEqual(self.events,[]);self.assertTrue(self.directory.exists())

    def test_replay_refuses_without_repeating_cleanup(self):
        self.run_cleanup();before=list(self.events)
        with self.assertRaises(RuntimeError):self.run_cleanup()
        self.assertFalse(self.directory.exists())
        self.assertEqual(self.events,before+['guard'])

    def test_loader_refuses_shadowed_original_guard_before_import(self):
        with patch.object(stopped.importlib.util,'find_spec',return_value=SimpleNamespace(origin='/wrong')), \
             patch.object(stopped.importlib,'import_module') as imported:
            with self.assertRaises(RuntimeError):stopped.load_dependencies(self.parent)
            imported.assert_not_called()

    def test_deadline_exhaustion_refuses_before_absence_commands(self):
        verified=stopped.validate(self.b)
        with patch.object(stopped.time,'monotonic',side_effect=[100.,200.]):
            owner=stopped.StoppedOwner(self.b,verified,self.d)
            with self.assertRaises(RuntimeError):owner.stopped_owner(self.owner,'normal')
        self.assertEqual(self.events,['guard'])

    def test_scope_cannot_borrow_signals_or_guard_release(self):
        self.b['allowedActions']['guardReleases']=1
        with self.assertRaises(RuntimeError):self.run_cleanup()
        self.assertEqual(self.events,[])

    def test_actual_alternate_interpreter_refuses_before_cleanup_imports(self):
        child=subprocess.run(['/usr/bin/python3','-B',stopped.__file__,'--help'],
            capture_output=True,text=True,timeout=20)
        self.assertNotEqual(child.returncode,0)
        self.assertIn('Original harness clock required before cleanup imports',child.stderr)
        self.assertNotIn('TypeError',child.stderr);self.assertEqual(child.stdout,'')


class HostObservationTests(unittest.TestCase):
    def test_host_commands_have_finite_bound_and_reject_unexpected_status_or_stderr(self):
        host=stopped.HostObservations()
        with patch.object(stopped.subprocess,'run',return_value=SimpleNamespace(returncode=1,stderr='',stdout='')) as run:
            self.assertEqual(host.command(['FAKE'],(1,)),'')
            self.assertEqual(run.call_args.kwargs['timeout'],10)
        with patch.object(stopped.subprocess,'run',return_value=SimpleNamespace(returncode=0,stderr='bad',stdout='')):
            with self.assertRaises(RuntimeError):host.command(['FAKE'],(0,))

    def test_health_uses_cached_get_only_and_refuses_connected_or_retargeted_bridge(self):
        for serial in ({'host':'127.0.0.1','port':2345,'connected':False},
                       {'host':'127.0.0.1','port':2345,'connected':True},
                       {'host':'OTHER','port':2345,'connected':False}):
            calls=[]
            connection=SimpleNamespace(request=lambda *a,**kw:calls.append((a,kw)),
                getresponse=lambda:SimpleNamespace(status=200,read=lambda n:json.dumps({'serial':serial}).encode()),
                close=lambda:calls.append('closed'))
            with patch.object(stopped.http.client,'HTTPConnection',return_value=connection):
                if serial['connected'] is False and serial['host']=='127.0.0.1':
                    self.assertEqual(stopped.HostObservations().health(),serial)
                else:
                    with self.assertRaises(RuntimeError):stopped.HostObservations().health()
            self.assertEqual(calls[0][0],('GET','/health'));self.assertEqual(calls[-1],'closed')


if __name__=='__main__':unittest.main()
