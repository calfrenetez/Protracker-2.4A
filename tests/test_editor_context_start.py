"""Private V3 start/freeze evidence and fake target/service observations only."""
import copy
from contextlib import contextmanager
import json
import os
from pathlib import Path
import sys
import subprocess
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import uuid

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import editor_context_start as start
import editor_context_runtime as runtime
import editor_context_binding as bindings
import editor_context_probe as probe
import editor_context_settlement as full
import editor_context_volume as volume
import editor_pointer_parent as original
import editor_pointer_binding as old_start
import test_editor_context_lifecycle as fixtures


class ContextStartTests(unittest.TestCase):
    prepare = fixtures.ContextLifecycleTests.prepare
    setUp = fixtures.ContextLifecycleTests.setUp

    def record(self):
        # These records are explicit HOST fixtures, never actual human approval.
        ready = volume.save(self.root / 'HOST-READY.json', {
            'state': 'ROOT_ACTUALLY_VERIFIED_HUMAN_APPROVED_DEVBENCH_READY', 'acceptedByRoot': True,
            'authorizationRef': volume.save(self.root / 'HOST-auth.json', {'HOST_ONLY': True}),
            'restartIntentRef': volume.save(self.root / 'HOST-intent.json', {'HOST_ONLY': True}),
            'restartResultRef': volume.save(self.root / 'HOST-result.json', {'HOST_ONLY': True}),
            'observed': {'job': {'pid': '555'}}})
        self.freeze['actualApprovedServiceReadinessRef'] = ready
        path = Path(self.b['sourceFreezeRef']['path']); path.write_text(json.dumps(self.freeze))
        freeze_ref = original.read(path)[1]; self.b['sourceFreezeRef'] = self.p['sourceFreezeRef'] = freeze_ref
        # Remove only this fixture's prospective parent so the actual-start
        # writer must publish new bindings rather than adopt its test setup.
        Path(self.parent_ref['path']).unlink()
        self.start_record = {'state': start.STATE, 'acceptedByRoot': True,
            'allowedActions': dict(start.ALLOWED), 'limits': dict(start.LIMITS),
            'windowDirectory': str(self.window), 'sourceFreezeRef': freeze_ref,
            'contextPreparationRef': self.p['contextPreparationRef'],
            'reservationRef': volume.save(self.window / 'reserve-result.json', {
                'state': 'FRESH_OWN_RESERVATION_NO_LAUNCH_OR_INPUT', 'run_id': 'fixture-owner',
                'window': str(self.window), 'target': 'amiberry-030', 'serviceReadinessRef': ready}),
            'startupResultRef': volume.save(self.window / 'startup-result.json', {'returncode': 0}),
            'ownedProcessRef': volume.save(self.window / 'ROOT_CURRENT_STARTED_PROCESS.json', self.owned),
            'serviceProcessRef': volume.save(self.window / 'ROOT_CURRENT_DEVBENCH_PROCESS.json', self.service),
            'nativeViewRef': volume.save(self.window / 'ROOT_NATIVE_WORKBENCH_VIEW.json', {
                'actuallyViewed': True, 'pid': self.owned['pid'], 'applicationVisible': False}),
            'originalProfileRef': self.p['originalProfileBeforeimage'], 'activeProfileRef': self.active_ref,
            'preservationBeforeRef': volume.save(self.window / 'fresh-preservation-before.json', {'HOST_ONLY': True}),
            'preparationRef': self.p['capturePreparationRef'], 'probeNativeRef': self.p['nativeCandidate'],
            'probeSourceRef': self.p['nativeSource'], 'probeCompileRef': self.p['compileReceipt'], 'runLabel': self.label}
        self.guest_error = False
        def guest(*args):
            self.events.append('guest')
            if self.guest_error: raise RuntimeError('HOST Guest admission failed')
        self.d.guest = guest
        self.d.extras = {'fresh_preservation': SimpleNamespace(verify_current=lambda before: {'HOST_ONLY': True})}
        return self.start_record

    def test_distinct_start_publishes_one_cpu_admission_and_two_exact_bindings(self):
        ref = start.execute(self.record(), self.d)
        b = json.loads(original.pinned(ref)); bindings.validate(b)
        self.assertEqual(ref['path'], str(self.window / 'context-parent-binding.json'))
        self.assertEqual(self.events.count('guest'), 1)
        self.assertEqual(self.events.count('operation-enter'), 1)
        self.assertEqual(b['allowedActions']['mouseEvents'], 0)
        self.assertFalse((self.window / self.label).exists())
        self.assertEqual(json.loads((self.window / 'capture-binding.json').read_text()), b['blankBinding'])
        with self.assertRaises(RuntimeError): start.execute(self.start_record, self.d)
        self.assertEqual(self.events.count('guest'), 1)

    def test_guest_failure_retains_intent_and_cannot_retry_or_publish_parent(self):
        record = self.record(); self.guest_error = True
        with self.assertRaisesRegex(RuntimeError, 'HOST Guest'): start.execute(record, self.d)
        self.assertTrue((self.window / 'mounted-guest-admission-intent.json').exists())
        self.assertFalse((self.window / 'context-parent-binding.json').exists())
        with self.assertRaises(RuntimeError): start.execute(record, self.d)
        self.assertEqual(self.events.count('guest'), 1); self.assertIn('operation-error', self.events)

    def test_changed_profile_prevents_operation_and_cpu_admission(self):
        record = self.record(); self.active = self.original
        with self.assertRaises(RuntimeError): start.execute(record, self.d)
        self.assertEqual(self.events, [])

    def test_changed_owner_prevents_intent_and_operation(self):
        record = self.record(); self.condition = 'owner'
        with self.assertRaises(RuntimeError): start.execute(record, self.d)
        self.assertFalse((self.window / 'mounted-guest-admission-intent.json').exists())
        self.assertNotIn('operation-enter', self.events)

    def test_service_refusal_prevents_intent_and_cpu_admission(self):
        record = self.record()
        def changed(*args): raise RuntimeError('HOST service changed')
        self.d.current_service = changed
        with self.assertRaises(RuntimeError): start.execute(record, self.d)
        self.assertFalse((self.window / 'mounted-guest-admission-intent.json').exists())
        self.assertNotIn('guest', self.events)

    def test_service_change_during_guest_prevents_publication(self):
        record = self.record(); calls = []
        def service(*args):
            calls.append(True)
            if len(calls) == 3: raise RuntimeError('HOST service changed after Guest')
        self.d.current_service = service
        with self.assertRaises(RuntimeError): start.execute(record, self.d)
        self.assertEqual(self.events.count('guest'), 1)
        self.assertFalse((self.window / 'context-parent-binding.json').exists())

    def test_profile_drift_during_guest_prevents_binding_publication(self):
        record = self.record()
        def guest(*args): self.events.append('guest'); self.active = self.original
        self.d.guest = guest
        with self.assertRaises(RuntimeError): start.execute(record, self.d)
        self.assertEqual(self.events.count('guest'), 1)
        self.assertFalse((self.window / 'context-parent-binding.json').exists())

    def test_whole_binder_deadline_cannot_be_extended_by_guest(self):
        record = self.record(); now = [100.]
        def guest(*args): self.events.append('guest'); now[0] += 61
        self.d.guest = guest
        with patch.object(start.time, 'monotonic', side_effect=lambda: now[0]):
            with self.assertRaises(RuntimeError): start.execute(record, self.d)
        self.assertEqual(self.events.count('guest'), 1)
        self.assertFalse((self.window / 'context-parent-binding.json').exists())

    def test_hold_or_inflight_does_not_admit_cpu(self):
        record = self.record(); self.phase = 'HOLD'
        with self.assertRaises(RuntimeError): start.execute(record, self.d)
        self.assertNotIn('guest', self.events)

    def test_inherited_frame_is_refused(self):
        with patch.dict(os.environ, {'AMIGA_LAUNCH_INFLIGHT': 'HOST_FOREIGN_FRAME'}):
            with self.assertRaises(RuntimeError): start.execute(self.record(), self.d)
        self.assertEqual(self.events, [])

    def test_v2_binder_refuses_v3_scope(self):
        record = self.record()
        with self.assertRaises(RuntimeError): old_start.validate_readonly_actual(record)
        record['state'] = old_start.READONLY_BIND
        with self.assertRaises(RuntimeError): start.validate_actual(record)

    def test_wrong_native_reference_is_not_adopted(self):
        record = self.record(); record['probeNativeRef'] = record['probeSourceRef']
        with self.assertRaises(RuntimeError): start.execute(record, self.d)
        self.assertEqual(self.events, [])

    def test_unviewed_startup_cannot_admit_guest(self):
        record = self.record(); ref = record['nativeViewRef']; path = Path(ref['path'])
        value = json.loads(path.read_text()); value['actuallyViewed'] = False
        path.write_text(json.dumps(value)); record['nativeViewRef'] = original.read(path)[1]
        with self.assertRaises(RuntimeError): start.execute(record, self.d)
        self.assertEqual(self.events, [])

    def test_runtime_loader_refuses_wrong_clock_before_importing_target_sources(self):
        with patch.object(original, 'require_harness_runtime', side_effect=RuntimeError('HOST clock refusal')):
            with patch.object(runtime.selected, 'checked_module', side_effect=AssertionError('No import')):
                with self.assertRaisesRegex(RuntimeError, 'clock refusal'): runtime.load(self.freeze)

    def test_runtime_commands_reject_system_python_before_project_imports(self):
        for name in ('editor_context_start.py', 'editor_context_parent.py', 'editor_context_dispatch.py'):
            with self.subTest(name=name):
                result = subprocess.run(['/usr/bin/python3', '-B', str(original.TOOLS / name),
                                         '--binding-ref', '{}'], capture_output=True, text=True, timeout=10)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Original harness clock required', result.stderr)
                self.assertNotIn('editor_pointer_parent.py', result.stderr)

    def test_runtime_loader_routes_selected_original_api_guard_guest_and_blank(self):
        self.record()
        modules = {name: SimpleNamespace() for name in self.freeze['modulePaths']}
        modules['launcher_guard'].LauncherGuard = self.d.guard
        for name in ('lease_input', 'install_client', 'install_ipc', 'operation'):
            setattr(modules['launcher_access'], name, getattr(self.d, name))
        modules['capture_runner'].OriginalAPI = self.d.original_api
        modules['shared_guest'].Guest = self.d.guest
        modules['editor_context_blank'] = bindings.blank
        observer = lambda: {'HOST_ONLY': True}
        validator = lambda *args: None
        extras = {'service_readiness': SimpleNamespace(observe_current=observer, validate_records=validator)}
        with patch.object(runtime.selected, 'checked_module', side_effect=lambda name, path, sources: modules[name]) as checked:
            with patch.object(bindings, 'load_extras', return_value=extras):
                loaded = runtime.load(self.freeze)
        self.assertEqual(checked.call_count, len(self.freeze['modulePaths']))
        self.assertIs(loaded.original_api, self.d.original_api); self.assertIs(loaded.guest, self.d.guest)
        self.assertIs(loaded.blank_case, bindings.blank.run_case)
        with patch.object(runtime, 'require_current_service', return_value={'HOST_ONLY': True}) as current:
            self.assertEqual(loaded.current_service(self.b), {'HOST_ONLY': True})
        self.assertEqual(current.call_args.args, (self.freeze, observer, validator))

    def test_generated_binding_integrates_full_normal_restoration_and_public_release(self):
        ref = start.execute(self.record(), self.d)
        self.b = json.loads(original.pinned(ref)); self.p = self.b['passiveProbe']; self.parent_ref = ref
        save = volume.save
        def existing_or_save(path, value):
            if Path(path).exists():
                self.assertEqual(json.loads(Path(path).read_text()), value)
                return original.read(path)[1]
            return save(path, value)
        with patch.object(volume, 'save', side_effect=existing_or_save):
            binding = fixtures.ContextLifecycleTests.full_binding(self)
        result = full.execute(binding, self.d)
        self.assertEqual(result['restoredRoles'], 3); self.assertEqual(result['appDataFiles'], 31)
        self.assertEqual((self.restore_calls, self.release_calls), (1, 1))

    def freeze_plan(self):
        plan = dict(self.freeze, state=start.PLAN, acceptedByRoot=True,
                    serviceReadinessRef=volume.save(self.root / 'HOST-plan-ready.json', {'HOST_ONLY': True}),
                    newWindow='/private/tmp/pt24g-context-not-created-' + uuid.uuid4().hex)
        Path(self.b['sourceFreezeRef']['path']).unlink()
        @contextmanager
        def locks():
            self.events.append('both-locks')
            with original.command_lock(self.h / 'runtime/test.lock'), original.command_lock(self.h / 'runtime/launcher-transport.lock'): yield
        def approved(args): self.events.append('actual-approved-service-required')
        self.freeze_d = {'canonical_profile_session_files': SimpleNamespace(locks=locks),
                         'service_readiness': SimpleNamespace(require_human_approved_service=approved)}
        return plan

    def test_fresh_v3_freeze_requires_both_locks_and_approved_epoch(self):
        plan = self.freeze_plan(); ref = start.freeze_sources(plan, self.freeze_d)
        value = json.loads(original.pinned(ref))
        self.assertEqual(value['state'], probe.FROZEN_STATE)
        self.assertEqual(self.events, ['both-locks', 'actual-approved-service-required'])
        self.assertFalse(Path(plan['newWindow']).exists()); self.assertFalse(value['runtimeAdmission'])
        with self.assertRaises(RuntimeError): start.freeze_sources(plan, self.freeze_d)

    def test_freeze_refuses_missing_approval_without_freeze_or_window_creation(self):
        plan = self.freeze_plan()
        def refused(args): raise RuntimeError('HOST approval missing')
        self.freeze_d['service_readiness'].require_human_approved_service = refused
        with self.assertRaises(RuntimeError): start.freeze_sources(plan, self.freeze_d)
        self.assertFalse((self.root / 'SOURCE-FREEZE.json').exists()); self.assertFalse(Path(plan['newWindow']).exists())


class ServiceObservationTests(unittest.TestCase):
    def setUp(self):
        self.events = []
        self.ref = {'path': '/fixture/ready'}
        self.ready = {'authorizationRef': {'path': '/fixture/auth'}, 'restartIntentRef': {'path': '/fixture/intent'},
                      'restartResultRef': {'path': '/fixture/result'},
                      'observed': {'job': {'pid': '555', 'runs': '8', 'program': 'HOST_SERVICE'},
                                   'processIdentity': 'HOST exact launch time and command'}}
        self.current = dict(self.ready['observed'], listenerPID=555,
            serial={'connected': True, 'host': '127.0.0.1', 'port': 2345}, noRecoveryActivation=True)
        self.freeze = {'actualApprovedServiceReadinessRef': self.ref}; self.drift = False
    def pin(self, ref):
        if self.drift: raise RuntimeError('HOST readiness metadata changed')
        return json.dumps(self.ready if ref == self.ref else {'HOST_ONLY': ref['path']}).encode()
    def observe(self): self.events.append('observe'); return self.current
    def validate(self, ready, auth, intent, result, historical):
        self.events.append('validate-approval-chain'); self.assertEqual(historical, self.ready['observed'])
    def check(self): return runtime.require_current_service(self.freeze, self.observe, self.validate, pin=self.pin)
    def test_same_approved_live_service_is_observed_without_reusing_historical_guard(self):
        self.assertEqual(self.check(), self.current)
        self.assertEqual(self.events, ['validate-approval-chain', 'observe'])
    def test_changed_live_identity_endpoint_or_recovery_refuses(self):
        for key, value in [('job', {'pid': '556'}), ('processIdentity', 'changed'), ('listenerPID', 556),
            ('listenerPID', True), ('serial', {'connected': False, 'host': '127.0.0.1', 'port': 2345}),
            ('serial', {'connected': True, 'host': 'foreign', 'port': 2345}), ('noRecoveryActivation', False)]:
            with self.subTest(key=key, value=value):
                before = self.current[key]; self.current[key] = value
                with self.assertRaises(RuntimeError): self.check()
                self.current[key] = before
    def test_invalid_approval_chain_prevents_live_observation(self):
        def refused(*args): raise RuntimeError('HOST approval chain refused')
        with self.assertRaises(RuntimeError): runtime.require_current_service(self.freeze, self.observe, refused, pin=self.pin)
        self.assertEqual(self.events, [])
    def test_readiness_pin_rechecked_after_observation(self):
        def observe(): self.drift = True; return self.current
        with self.assertRaises(RuntimeError): runtime.require_current_service(self.freeze, observe, self.validate, pin=self.pin)


if __name__ == '__main__': unittest.main()
