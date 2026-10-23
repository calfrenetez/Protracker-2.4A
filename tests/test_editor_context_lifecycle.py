"""Real private files, original guard/restorer; synthetic host/guest observations.

Only temporary fixture locks/reservations are exercised. No shared lifecycle,
real transport, GUI input, service change or target access occurs.
"""
import asyncio
import copy
from contextlib import contextmanager
import importlib.util
import json
import os
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace, ModuleType
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import editor_pointer_parent as original
import editor_pointer_binding as saved
import editor_context_binding as binder
import editor_context_parent as parent
import editor_context_blank as blank
import editor_context_probe as probe
import editor_context_retention as retention
import editor_context_stopped as stopped
import editor_context_settlement as full
import editor_probe_stopped as original_stopped
import editor_capture_settlement as original_full
import editor_probe_namespace as ns
import editor_capture_namespace as app_ns
import editor_context_volume as volume
from editor_blank_capture_protocol import recent_line, BLANK_FRAME
import test_editor_context_volume as fixtures
import test_editor_context_probe as probe_fixtures


class ContextLifecycleTests(unittest.TestCase):
    prepare = fixtures.ProbeVolumeTests.prepare

    def setUp(self):
        fixtures.ProbeVolumeTests.setUp(self)
        self.wtemp = tempfile.TemporaryDirectory(prefix='pt24g-context-window-HOST-', dir='/private/tmp')
        self.addCleanup(self.wtemp.cleanup); self.window = Path(self.wtemp.name)
        self.label = 'blank-capture-context-host'; self.out = self.window / self.label
        self.h = self.root / 'HOST-harness'
        (self.h / 'runtime/Dev/Tests').mkdir(parents=True)
        (self.h / 'runtime/launcher-guard').mkdir()
        for name in ('test.lock', 'launcher-transport.lock'): (self.h / 'runtime' / name).touch(mode=0o600)
        self.events = []; self.phase = 'READY'; self.condition = None; self.released = False
        self.release_calls = self.restore_calls = 0
        for module in (ns, app_ns, stopped, original_stopped, full):
            p = patch.object(module, 'HARNESS', self.h); p.start(); self.addCleanup(p.stop)
        lock = original.command_lock
        @contextmanager
        def local_lock(path):
            if str(path) == str(original.HARNESS / 'runtime/test.lock'): path = self.h / 'runtime/test.lock'
            with lock(path) as fd: yield fd
        p = patch.object(original, 'command_lock', local_lock); p.start(); self.addCleanup(p.stop)
        prepared_ref = self.prepare(); prepared = volume.verify(prepared_ref)
        self.active = self.original + prepared['appendASCII'].encode()
        self.active_ref = {'path': str(original.HARNESS / 'runtime/amiberry/Configurations/A1200-030-DEV.uae'),
                           'HOST_SYNTHETIC': True}
        pinned = original.pinned
        def pin(ref):
            if ref['path'] == self.active_ref['path']:
                if ref != self.active_ref: raise RuntimeError('HOST wrong active ref')
                return self.active
            return pinned(ref)
        p = patch.object(original, 'pinned', pin); p.start(); self.addCleanup(p.stop)
        for name in ('NATIVE', 'SOURCE', 'EDITOR'):
            p = patch.object(probe, name, getattr(volume, name)); p.start(); self.addCleanup(p.stop)
        editor = volume.pinned(self.refs['capturePreparation'])
        capture = json.loads(editor); editor_body = volume.pinned(capture['originalCandidate'])
        for name, value in (('CANDIDATE_BYTES', len(editor_body)), ('CANDIDATE_SHA', original.digest(editor_body)[1])):
            p = patch.object(blank, name, value); p.start(); self.addCleanup(p.stop)
        # Existing private source is read as a reviewed template, never executed
        # as its historical transaction. All new runtime references use this fixture.
        historical = Path('/private/tmp/pt24g-editor-readonly-session-20261023-r3')
        modules = {}; sources = {}
        local = {'capture_runner', 'capture_case_mounted', 'first_mouse_api', 'capture_support', 'root_observation'}
        shared = {'launcher_guard', 'launcher_access', 'bridge_checks', 'shared_guest', 'emulator'}
        for name in binder.MODULES:
            if name in local:
                path = self.root / (name + '.py')
                body = (Path(blank.__file__) if name == 'capture_case_mounted' else historical / path.name).read_bytes()
                volume.put(path, body)
            elif name in shared: path = original.HARNESS / 'scripts' / (name + '.py')
            elif name.startswith('amiberry_mcp.'):
                path = original.HARNESS / '.venv/lib/python3.14/site-packages/amiberry_mcp' / (name.split('.')[-1] + '.py')
            else: path = original.TOOLS / (name + '.py')
            modules[name] = str(path); sources[str(path)] = original.read(path)[1]
        extras = {}
        for name in binder.EXTRA:
            path = self.root / (name + '.py'); volume.put(path, (historical / path.name).read_bytes())
            extras[name] = str(path); sources[str(path)] = original.read(path)[1]
        for name in ('editor_pointer_parent', 'editor_pointer_binding', 'editor_probe_stopped',
                     'editor_capture_settlement', 'editor_capture_namespace'):
            path = original.TOOLS / (name + '.py'); sources[str(path)] = original.read(path)[1]
        self.freeze = {'state': probe.FROZEN_STATE, 'sources': sources, 'modulePaths': modules,
                       'extraModulePaths': extras, 'contextModulePaths': probe.MODULES,
                       'newWindow': str(self.window), 'contextPreparationRef': prepared_ref}
        freeze_ref = volume.save(self.root / 'SOURCE-FREEZE.json', self.freeze)
        before = volume.put(self.window / 'before-canonicalProfile.bin', self.original)
        self.p = {'state': probe.INPUT_STATE, 'acceptedByRoot': True, 'target': 'amiberry-030',
                  'ownerRunID': 'fixture-owner', 'sourceFreezeRef': freeze_ref, 'sourcePins': sources,
                  'allowedActions': dict(probe.ALLOWED), 'limits': dict(probe.LIMITS),
                  'windowDirectory': str(self.window), 'runLabel': self.label,
                  'contextPreparationRef': prepared_ref, 'capturePreparationRef': self.refs['capturePreparation'],
                  'nativeSource': self.refs['nativeSource'], 'nativeCandidate': self.refs['nativeCandidate'],
                  'compileReceipt': self.refs['compileReceipt'], 'stagedCandidate': prepared['candidate'],
                  'editorCandidate': capture['privateCandidate'], 'guestCandidate': 'PTContext:PTEditorPointerContextV3',
                  'activeProfile': self.active_ref, 'originalProfileBeforeimage': before}
        self.owned = {'pid': 999999, 'argv': ['999999', '999999', 'HOST_VM'], 'socket_inode': 123}
        self.service = {'pid': 555, 'argv': ['555', '555', 'HOST_SERVICE']}
        c = {'state': blank.BLANK_STATE, 'acceptedByRoot': True, 'target': 'amiberry-030',
             'ownerRunID': 'fixture-owner', 'sourcePins': sources, 'modulePaths': modules,
             'allowedActions': dict(blank.ALLOWED), 'limits': dict(blank.LIMITS),
             'expectedHistoryBytes': blank.HISTORY_BYTES, 'candidate': capture['originalCandidate'],
             'mountedCandidate': capture['privateCandidate'], 'guestCandidate': capture['guestCandidate'],
             'localVolumePreparation': self.refs['capturePreparation'], 'activeProfile': self.active_ref,
             'originalProfileBeforeimage': before, 'readonlyProbe': self.p, 'outputDirectory': str(self.out),
             'ownedProcess': self.owned}
        self.b = {'state': binder.STATE, 'acceptedByRoot': True, 'target': 'amiberry-030',
                  'ownerRunID': 'fixture-owner', 'sourcePins': sources, 'sourceFreezeRef': freeze_ref,
                  'allowedActions': dict(binder.ALLOWED), 'limits': dict(binder.LIMITS),
                  'blankBinding': c, 'passiveProbe': self.p, 'runLabel': self.label,
                  'mountedCandidate': capture['privateCandidate'], 'outputDirectory': str(self.out)}
        self.parent_ref = volume.save(self.window / 'context-parent-binding.json', self.b)
        outer = self
        class Guard:
            def __init__(self, root): pass
            def status(self):
                return {'phase': 'EMPTY' if outer.released else outer.phase,
                        'reservation': None if outer.released else {'lease': {'run_id': 'fixture-owner'}},
                        'holds': [{}] if outer.phase == 'HOLD' and not outer.released else [],
                        'inflight': None, 'fence': 7}
            @contextmanager
            def _locked(self): yield None, {}
            def _owner(self, state, lease, target):
                if outer.condition == 'owner' or lease['run_id'] != 'fixture-owner':
                    raise RuntimeError('HOST changed owner')
            def release(self, lease, proof):
                outer.release_calls += 1
                cleanup = json.loads((outer.window / 'cleanup-release-fact.json').read_text())
                facts = cleanup['contextInputRetention']['facts']['contextInput']
                outer.assertEqual(facts['facts']['probeFilesRemoved'], 0)
                outer.assertEqual(original.pinned(outer.p['stagedCandidate']), outer.native)
                outer.assertFalse(outer.run.exists())
                outer.assertEqual(set(proof['records']), {'authorization', 'restoration', 'absence', 'cleanup'})
                outer.released = True
                history = outer.h / 'runtime/launcher-guard/history'; history.mkdir()
                (history / 'HOST-archive.json').write_text(json.dumps({'proof': proof,
                    'reservation': {'lease': lease}, 'holds': [{}] if outer.phase == 'HOLD' else []}))
                return {'released': True, 'archive': 'HOST-archive.json', 'fence': 8}
        class Host:
            def command(self, argv, expected):
                if '-p' in argv: return '555 555 OTHER' if outer.condition == 'service' else '555 555 HOST_SERVICE'
                if argv[0].endswith('pgrep'): return '999999' if outer.condition == 'vm' else ''
                if argv[0].endswith('lsof'): return 'listener' if outer.condition == 'port' else ''
                return '42 999999' if outer.condition == 'group' else '42 42'
            def socket_absent(self): return outer.condition != 'socket'
            def health(self): return {'host': '127.0.0.1', 'port': 2345, 'connected': outer.condition == 'bridge'}
        @contextmanager
        def operation(*args, **kwargs):
            outer.events.append('operation-enter')
            try: yield {'HOST_FRAME': True}
            except BaseException: outer.events.append('operation-error'); raise
            else: outer.events.append('operation-return')
        class API:
            pinned = staticmethod(pin); read = staticmethod(original.read)
            put = staticmethod(volume.put); save = staticmethod(volume.save)
            def __init__(self, binding, frame): self.binding, self.frame = binding, frame
            def owner(self, owner):
                if outer.condition == 'owner' or owner != 'fixture-owner': raise RuntimeError('HOST API owner loss')
            def clock(self): return 100.
        self.d = SimpleNamespace(guard=Guard, lease_input=lambda *args: {'run_id': 'fixture-owner'},
            observations=Host(), decode=lambda b: json.loads(b), operation=operation, original_api=API,
            current_service=lambda b: self.events.append('service'), install_client=lambda: None,
            install_ipc=lambda *args: None, blank_case=blank.run_case)
        self.api = API(c, {'HOST_FRAME': True})

    def blank_api(self, drift=False):
        outer = self
        class Guest:
            def __init__(self, harness, evidence):
                outer.events.append('Guest'); self.share = outer.h / 'runtime/Dev/Tests'
                self.device = 'Dev:Tests/'; self.launch = self.share / ('launch-' + outer.label); self.count = 0
            def start(self):
                self.count += 1; outer.events.append('start-' + str(self.count)); run = self.share / outer.label
                if self.count == 1:
                    settings, _ = blank.environment_script(self.device + outer.label)
                    for i, value in enumerate(settings.values()): (run / ('active-' + str(i))).write_bytes(value.encode())
                    for leaf in ('backup-ready', 'setup-done'): (run / leaf).write_bytes(b'ready\n')
                    (outer.out / 'launch.log').write_bytes(b'HOST setup')
                else: (run / 'editor.log').write_bytes(outer.log())
        api = self.api; api.Guest = Guest
        def prepare(guest, out):
            run = guest.share / outer.label; run.mkdir(); return run
        api.prepare_run = prepare
        api.require_running_guest = lambda *args: api.owner('fixture-owner')
        api.optional = lambda path, maximum: original.read(path, maximum)[0] if path.exists() else None
        api.put_unique_snapshot = lambda out, body: volume.put(out / 'HOST-observed.log', body)
        def replace(path, before, after):
            self.assertEqual(path.read_bytes(), before); path.write_bytes(after)
        api.replace = replace
        async def readback(guest, path):
            outer.events.append('editor-readback'); path.write_bytes(original.pinned(self.b['blankBinding']['candidate']))
            if drift: self.active = self.original
        async def capture(path): self.events.append('capture'); return {'HOST_CAPTURE': True}
        async def sleep(seconds): pass
        api.readback = readback; api.capture = capture; api.sleep = sleep
        return api

    def log(self):
        return recent_line(self.label) + b'\nEDITOR READY channels=4 patterns=1 bitmap_bytes=163840 history_bytes=182388\n' + BLANK_FRAME + b'\n'

    def captured(self):
        self.out.mkdir(); (self.out / 'editor-after-capture.log').write_bytes(self.log())
        run = self.h / 'runtime/Dev/Tests' / self.label; run.mkdir(); (run / 'editor.log').write_bytes(self.log())
        return {'state': 'BLANK_LOG_AND_REGISTERED_CAPTURE_RECORDED', 'ownerRunID': 'fixture-owner',
                'attempts': {'candidate-readback': 1, 'environment-setup': 1, 'application-launch': 1, 'capture': 1}}

    def carrier(self, action=None):
        outer = self
        class IO:
            async def readback(self, guest, path):
                outer.events.append('native-readback'); path.write_bytes(outer.native)
            async def snapshot(self, guest, out):
                outer.events.append('native-execute')
                if action: action()
                return probe_fixtures.stdout()
        return probe.EditorPointerContextProbe(self.p, self.api, io=IO())

    def run_diagnostic(self, captured, carrier=None, deadline=460.):
        # Only the original shared guest-log root is replaced by the fixture root.
        with patch.object(original, 'HARNESS', self.h):
            return asyncio.run(parent.run_same_frame(self.b, captured, self.api,
                deadline=deadline, carrier=carrier or self.carrier()))

    def test_exact_source_and_stopped_safe_input_do_not_require_active_profile(self):
        binder.validate(self.b); self.active = self.original
        binder.validate(self.b); probe.prepared_input(self.p)
        with self.assertRaises(RuntimeError): probe.require_current_mounted_input(self.p, self.api)

    def test_original_v2_parent_and_stopped_and_full_scopes_refuse_v3(self):
        with self.assertRaises(RuntimeError): original.validate_binding(self.b, readonly=True)
        binding = self.full_binding()
        with self.assertRaises(RuntimeError): original_stopped.validate(json.loads(original.pinned(binding['stoppedBindingRef'])))
        with self.assertRaises(RuntimeError): original_full.validate(binding)

    def test_distinct_parent_scope_rejects_input_expansion(self):
        self.b['allowedActions']['mouseEvents'] = 1
        with self.assertRaises(RuntimeError): binder.validate(self.b)
        self.assertEqual(self.events, [])

    def test_blank_one_readback_setup_launch_capture_and_no_input(self):
        result = asyncio.run(blank.run_case(self.b['blankBinding'], self.out, self.blank_api()))
        self.assertEqual(result['attempts'], {'candidate-readback': 1, 'environment-setup': 1, 'application-launch': 1, 'capture': 1})
        self.assertEqual(self.events, ['Guest', 'editor-readback', 'start-1', 'start-2', 'capture'])
        self.assertFalse(result['cleanExitClaimed'])

    def test_blank_profile_drift_retains_failure_and_prevents_launch(self):
        with self.assertRaises(RuntimeError): asyncio.run(blank.run_case(self.b['blankBinding'], self.out, self.blank_api(True)))
        self.assertEqual(self.events, ['Guest', 'editor-readback'])
        self.assertEqual(json.loads((self.out / 'result.json').read_text())['state'], 'CAPTURE_CASE_FAILED_RETAINED')

    def test_one_passive_context_in_original_api_frame_never_adjusts_mismatch(self):
        result = self.run_diagnostic(self.captured())
        self.assertEqual(self.events, ['native-readback', 'native-execute'])
        self.assertEqual(result['contextObservation']['window']['mouse_x'], 219)
        self.assertFalse(result['inputAuthorized']); self.assertFalse(result['selectionAccepted'])
        self.assertEqual(result['hostRaises'], 0)

    def test_log_drift_after_context_fails_with_no_retry_or_input(self):
        captured = self.captured()
        log = self.h / 'runtime/Dev/Tests' / self.label / 'editor.log'
        with self.assertRaises(ValueError): self.run_diagnostic(captured, self.carrier(lambda: log.write_bytes(b'changed')))
        self.assertEqual(self.events, ['native-readback', 'native-execute'])
        self.assertEqual(json.loads((self.out / 'context-parent-result.json').read_text())['state'], 'EDITOR_CONTEXT_V3_DIAGNOSTIC_FAILED_RETAINED')

    def test_parent_deadline_expired_prevents_native_readback(self):
        with self.assertRaises(RuntimeError): self.run_diagnostic(self.captured(), deadline=100.)
        self.assertEqual(self.events, [])

    def test_same_original_api_binding_required(self):
        captured = self.captured(); self.api.binding = {}
        with self.assertRaises(RuntimeError): self.run_diagnostic(captured)
        self.assertEqual(self.events, [])

    def test_composition_uses_one_original_operation_and_same_frame(self):
        carrier = self.carrier(); real_run = parent.run_same_frame
        async def captured(binding, out, api):
            self.assertIs(api.frame, self.frame); self.events.append('blank')
            return self.captured()
        def api(binding, frame):
            self.frame = frame; self.api.frame = frame; return self.api
        async def run(binding, captured, api, **kw):
            self.assertIs(api, carrier.original)
            with patch.object(original, 'HARNESS', self.h):
                return await real_run(binding, captured, api, carrier=carrier, **kw)
        self.d.blank_case = captured; self.d.original_api = api
        with patch.object(parent, 'run_same_frame', side_effect=run):
            result = parent.execute(self.b, self.d)
        self.assertEqual(result['state'], 'EDITOR_CONTEXT_V3_DIAGNOSTIC_DATA_RECORDED_NO_INPUT')
        self.assertEqual(self.events.count('operation-enter'), 1)
        self.assertEqual(self.events.count('operation-return'), 1)
        self.assertEqual([v for v in self.events if v != 'service'],
                         ['operation-enter', 'blank', 'native-readback', 'native-execute', 'operation-return'])
        before = list(self.events)
        with self.assertRaises(RuntimeError): parent.execute(self.b, self.d)
        self.assertEqual(before, self.events)

    def test_parent_refuses_changed_profile_before_blank_guest_construction(self):
        self.active = self.original
        self.d.blank_case = lambda *args: self.fail('No blank Guest on a changed profile')
        with self.assertRaises(RuntimeError): parent.execute(self.b, self.d)
        self.assertNotIn('operation-enter', self.events)
        self.assertNotIn('native-readback', self.events)

    def test_blank_failure_propagates_through_original_operation_without_probe(self):
        async def fail(*args): raise RuntimeError('HOST failed blank capture')
        self.d.blank_case = fail
        with self.assertRaisesRegex(RuntimeError, 'HOST failed blank'): parent.execute(self.b, self.d)
        self.assertIn('operation-error', self.events); self.assertNotIn('native-readback', self.events)

    def test_pre_context_log_drift_prevents_native_readback(self):
        captured = self.captured()
        (self.h / 'runtime/Dev/Tests' / self.label / 'editor.log').write_bytes(b'changed')
        with self.assertRaises(ValueError): self.run_diagnostic(captured)
        self.assertEqual(self.events, [])

    def test_native_owner_loss_preserves_failure_without_input(self):
        captured = self.captured()
        def changed(): self.condition = 'owner'
        with self.assertRaises(RuntimeError): self.run_diagnostic(captured, self.carrier(changed))
        self.assertEqual(self.events, ['native-readback', 'native-execute'])
        self.assertEqual(json.loads((self.out / 'context-parent-result.json').read_text())['state'],
                         'EDITOR_CONTEXT_V3_DIAGNOSTIC_FAILED_RETAINED')

    def full_binding(self, mode='normal'):
        self.phase = 'HOLD' if mode == 'failed' else 'READY'
        sb = {'state': stopped.STATE, 'acceptedByRoot': True, 'target': 'amiberry-030',
              'mode': mode, 'limits': {'whole': 60}, 'allowedActions': dict(stopped.ALLOWED),
              'ownerRunID': 'fixture-owner', 'windowDirectory': str(self.window),
              'parentBindingRef': self.parent_ref,
              'guardRef': volume.put(self.h / 'runtime/launcher-guard/state.json', b'HOST_GUARD'),
              'stopReceiptRef': volume.save(self.window / 'ROOT_ACTUAL_OWNED_STOP.json',
                  {'ownerRunID': 'fixture-owner', 'pid': 999999, 'processAndGroupAbsent': True}),
              'ownedProcessRef': volume.save(self.window / 'ROOT_CURRENT_STARTED_PROCESS.json', self.owned),
              'serviceProcessRef': volume.save(self.window / 'ROOT_CURRENT_DEVBENCH_PROCESS.json', self.service)}
        self.run = self.h / 'runtime/Dev/Tests' / self.label; self.run.mkdir(); (self.run / 'copies').mkdir()
        (self.run / 'editor.log').write_bytes(b'HOST application evidence')
        (self.run.parent / ('launch-' + self.label)).write_bytes(b'HOST launcher')
        app = self.root / 'app-data'; app.mkdir()
        p = patch.object(full, 'APP_ROOT', app); p.start(); self.addCleanup(p.stop)
        rows = {}
        for name in sorted(full.THEMES) + ['Data/file%02d' % i for i in range(29)]:
            path = app / name; path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(name.encode())
            backup = self.root / 'saved-app-data' / name; backup.parent.mkdir(parents=True, exist_ok=True)
            backup.write_bytes(path.read_bytes())
            rows[name] = {'original': original.read(path)[1], 'backup': str(backup.relative_to(self.root))}
        self.rows = rows; self.roles = {}; scope = {}
        for role in ('canonicalProfile', 'emulatorState', 'bridgeEnabled'):
            path = self.h / 'runtime' / role; path.write_bytes(self.original if role == 'canonicalProfile' else role.encode())
            self.roles[role] = path; scope[role] = original.read(path)[1]
            if role != 'canonicalProfile': (self.window / ('before-' + role + '.bin')).write_bytes(path.read_bytes())
            path.write_bytes(b'HOST ACTIVE SESSION')
        self.full_b = {'state': full.STATE, 'acceptedByRoot': True, 'ownerRunID': 'fixture-owner',
            'windowDirectory': str(self.window), 'mode': mode, 'allowedActions': dict(full.ALLOWED), 'limits': {'whole': 180},
            'stoppedBindingRef': volume.save(self.window / 'probe-stopped-binding.json', sb),
            'appDataBeforeRef': volume.save(self.window / 'current-app-data-before-and-backups.json', {'files': rows}),
            'roleBaselineRef': volume.save(self.window / 'before-owned-lifecycle-files.json', scope),
            'preservationBeforeRef': volume.save(self.window / 'fresh-preservation-before.json', {'HOST_ONLY': True})}
        for name in full.THEMES:
            path = app / name; os.utime(path, ns=(path.stat().st_atime_ns, rows[name]['original']['fullFacts']['st_mtime_ns'] + 1000000))
        @contextmanager
        def locks():
            with original.command_lock(self.h / 'runtime/test.lock'), original.command_lock(self.h / 'runtime/launcher-transport.lock'): yield
        def restore(a, ap, window, action):
            self.restore_calls += 1; restored = {}
            with locks():
                for role, path in self.roles.items():
                    path.write_bytes((self.window / ('before-' + role + '.bin')).read_bytes())
                    os.utime(path, ns=(path.stat().st_atime_ns, scope[role]['fullFacts']['st_mtime_ns']))
                    restored[role] = original.read(path)[1]
                self.active = self.original
                path = self.window / 'restore-canonical-profile-session-result.json'
                volume.save(path, {'state': 'THREE_HOST_FILE_ORIGINAL_BODIES_RESTORED_AFTER_VERIFIED_OWNED_STOP', 'restoredFull4s': restored})
                return path
        self.d.files = SimpleNamespace(ROLES=self.roles, locks=locks, resident=lambda path, private=False: original.read(path),
            pinned=lambda ref, private=False: original.pinned(ref),
            admissions=lambda path, action: (json.loads(original.read(path)[0]), original.read(path)[1], self.window), execute=restore)
        self.d.preservation = SimpleNamespace(verify_current=lambda before: {'HOST_PROTECTED': True})
        return self.full_b

    def test_full_normal_restores_roles_all31_and_releases_once_retaining_v3_input(self):
        result = full.execute(self.full_binding(), self.d)
        self.assertEqual(result['state'], 'FULL_STOPPED_EDITOR_AND_PROBE_RESTORED_AND_RELEASED')
        self.assertEqual((self.restore_calls, self.release_calls), (1, 1)); self.assertEqual(result['appDataFiles'], 31)
        self.assertEqual(self.active, self.original); self.assertEqual(original.pinned(self.p['stagedCandidate']), self.native)
        with self.assertRaises(RuntimeError): full.execute(self.full_b, self.d)
        self.assertEqual(self.release_calls, 1)

    def test_full_failed_hold_archives_once_after_restoration(self):
        result = full.execute(self.full_binding('failed'), self.d)
        self.assertEqual(result['archivedHolds'], 1); self.assertEqual(self.release_calls, 1)
        self.assertEqual((self.window / 'preserved-readonly-context-input/PTEditorPointerContextV3').read_bytes(), self.native)

    def test_presence_service_or_owner_change_prevents_restoration_and_release(self):
        binding = self.full_binding('failed')
        for condition in ('owner', 'service', 'vm', 'group', 'socket', 'port', 'bridge'):
            with self.subTest(condition=condition):
                self.condition = condition
                with self.assertRaises(RuntimeError): full.execute(binding, self.d)
                self.assertFalse((self.window / 'preserved-readonly-context-input').exists())
                self.assertEqual((self.restore_calls, self.release_calls), (0, 0))

    def test_retention_drift_after_restoration_blocks_public_release(self):
        binding = self.full_binding()
        def drift(before):
            (self.window / 'preserved-readonly-context-input/PTEditorPointerContextV3').write_bytes(b'changed evidence')
            return {'HOST_ONLY': True}
        self.d.preservation.verify_current = drift
        with self.assertRaises(RuntimeError): full.execute(binding, self.d)
        self.assertEqual((self.restore_calls, self.release_calls), (1, 0))
        self.assertFalse((self.window / 'release-proof.json').exists())

    def test_changed_app_data_body_blocks_restoration_and_release(self):
        binding = self.full_binding()
        (full.APP_ROOT / 'Data/file00').write_bytes(b'foreign body')
        with self.assertRaises(RuntimeError): full.execute(binding, self.d)
        self.assertEqual((self.restore_calls, self.release_calls), (0, 0))
        self.assertEqual((full.APP_ROOT / 'Data/file00').read_bytes(), b'foreign body')

    def test_changed_context_volume_blocks_namespace_deletion_and_restoration(self):
        binding = self.full_binding(); Path(self.p['stagedCandidate']['path']).chmod(0o700)
        with self.assertRaises(RuntimeError): full.execute(binding, self.d)
        self.assertTrue(self.run.exists()); self.assertEqual((self.restore_calls, self.release_calls), (0, 0))

    def real_originals(self, mode):
        binding = self.full_binding(mode)
        helper = self.root / 'canonical_profile_session_files.py'; body = helper.read_bytes()
        self.assertEqual(original.digest(body), saved.SAVED_FILE_HELPER)
        f = ModuleType('HOST_context_original_files'); f.__file__ = str(helper)
        exec(compile(body, str(helper), 'exec'), f.__dict__)
        binder.configure_saved_files(f, self.b['sourcePins'], self.p['capturePreparationRef'],
                                     self.p['contextPreparationRef'], self.p['originalProfileBeforeimage'])
        self.assertIn(b'filesystem2=ro,DH4:PTContext:', f.APPEND)
        f.I = self.h; f.ROLES = self.roles; f.ORIGINAL = original.digest(self.original)
        self.roles['canonicalProfile'].write_bytes(self.original + f.APPEND)
        scope = json.loads(original.pinned(binding['roleBaselineRef']))
        for role in self.roles:
            backup = self.window / ('before-' + role + '.bin')
            if backup.stat().st_mode & 0o777 != 0o600: backup.chmod(0o600)
            scope[role]['absent'] = False
        path = Path(binding['roleBaselineRef']['path']); path.write_text(json.dumps(scope)); binding['roleBaselineRef'] = original.read(path)[1]
        volume.save(self.window / 'patch-canonical-profile-session-result.json',
                    {'state': 'CANONICAL_PROFILE_SESSION_APPEND_APPLIED_ORIGINAL_BYTES_PRESERVED'})
        spec = importlib.util.spec_from_file_location('HOST_context_original_guard', original.HARNESS / 'scripts/launcher_guard.py')
        guard_module = importlib.util.module_from_spec(spec); spec.loader.exec_module(guard_module)
        state = self.h / 'runtime/launcher-guard'; (state / 'state.json').unlink(); state.rmdir()
        guard = guard_module.initialize(self.h, [], {'mode': 'temporary-fixture', 'approved': True,
            'legacy_holds_verified': True, 'record_sha256': '1' * 64})
        lease = guard.reserve('amiberry-030', 'HOST only', 'Private V3 restoration fixture')
        if mode == 'failed': guard.hold(lease, 'HOST fixture failure; no target')
        owner = lease['run_id']; self.b['ownerRunID'] = owner
        self.b['blankBinding']['ownerRunID'] = owner; self.p['ownerRunID'] = owner
        path = Path(self.parent_ref['path']); path.write_text(json.dumps(self.b)); self.parent_ref = original.read(path)[1]
        sb = json.loads(original.pinned(binding['stoppedBindingRef']))
        sb.update(ownerRunID=owner, parentBindingRef=self.parent_ref, guardRef=original.read(state / 'state.json')[1])
        path = self.window / 'ROOT_ACTUAL_OWNED_STOP.json'
        path.write_text(json.dumps({'ownerRunID': owner, 'pid': 999999, 'processAndGroupAbsent': True}))
        sb['stopReceiptRef'] = original.read(path)[1]
        path = Path(binding['stoppedBindingRef']['path']); path.write_text(json.dumps(sb))
        binding.update(ownerRunID=owner, stoppedBindingRef=original.read(path)[1])
        self.d.guard = guard_module.LauncherGuard; self.d.lease_input = lambda *args: lease
        self.d.decode = guard_module._decode; self.d.files = f
        return binding, guard

    def test_original_guard_and_original_full_v3_restoration_normal_private_fixture(self):
        binding, guard = self.real_originals('normal'); result = full.execute(binding, self.d)
        self.assertEqual(guard.status()['phase'], 'EMPTY'); self.assertEqual(result['archivedHolds'], 0)
        self.assertEqual(self.roles['canonicalProfile'].read_bytes(), self.original)

    def test_original_guard_and_original_full_v3_restoration_failed_private_fixture(self):
        binding, guard = self.real_originals('failed'); result = full.execute(binding, self.d)
        self.assertEqual(guard.status()['phase'], 'EMPTY'); self.assertEqual(result['archivedHolds'], 1)
        self.assertEqual(original.pinned(self.p['stagedCandidate']), self.native)


if __name__ == '__main__': unittest.main()
