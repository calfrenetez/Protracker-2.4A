"""Synthetic finite carrier tests; no harness import, service or target access.

Only fixture content constants are substituted. The production parser and
carrier run unchanged, with real temporary evidence files and exclusive writes.
"""
import asyncio
from contextlib import asynccontextmanager
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import time
from types import ModuleType, SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import editor_pointer_probe as probe
from editor_pointer_snapshot import SCREEN_TITLE

NATIVE = b'synthetic native bytes'
SOURCE = b'synthetic source bytes'
EDITOR = b'synthetic editor bytes'


def ref(path, body):
    return {'path': str(path), 'bytes': len(body),
            'sha256': hashlib.sha256(body).hexdigest(), 'fullFacts': {'fixture': True}}


def stdout():
    return ('[OK]\nPT_EDITOR_SCREEN_V2 screen_width=640 screen_height=512 mouse_x=219 mouse_y=425 '
            'left=0 top=0 viewport_dx=0 viewport_dy=0 viewport_modes=32772 screen_flags=15 '
            'default_title_state=2 default_title_hex=' + SCREEN_TITLE.encode().hex() + '\n'
            'PT_EDITOR_WINDOW_V2 left=0 top=0 width=640 height=512 mouse_x=219 mouse_y=425 '
            'border_left=0 border_top=0 border_right=0 border_bottom=0 window_flags=2304 '
            'title_state=0 title_hex=-\n').encode()


class FakeOriginal:
    def __init__(self):
        self.now = 100.
        self.inputs = {}
        self.events = []
        self.alive = True
        self.drift = False

    def clock(self): return self.now

    def owner(self, owner):
        if not self.alive or owner != 'fixture-owner':
            raise RuntimeError('synthetic owner loss')

    def pinned(self, pin):
        if self.drift:
            raise RuntimeError('synthetic input drift')
        path = pin['path']
        body = self.inputs[path] if path in self.inputs else Path(path).read_bytes()
        if ref(path, body) != pin:
            raise RuntimeError('synthetic exact pin changed')
        return body

    def read(self, path):
        body = Path(path).read_bytes()
        return body, ref(path, body)

    def save(self, path, value):
        with Path(path).open('x') as stream:
            json.dump(copy.deepcopy(value), stream)


class FakeIO:
    def __init__(self, original):
        self.original = original
        self.option = None

    async def readback(self, guest, path):
        self.original.events.append(('readback', guest))
        self.path = path
        path.write_bytes(b'wrong' if self.option == 'wrongbytes' else NATIVE)
        if self.option == 'readback-error': raise RuntimeError('synthetic transport failure')
        if self.option == 'owner-loss': self.original.alive = False
        if self.option == 'pin-change': self.original.drift = True
        if self.option == 'aggregate-deadline': self.original.now += 21
        if self.option == 'parent-deadline': self.original.now += 2
        if self.option == 'timeout': await asyncio.sleep(10)
        if self.option == 'cancel': raise asyncio.CancelledError()

    async def snapshot(self, guest, out):
        self.original.events.append(('execute', guest))
        if self.option == 'execute-error': raise RuntimeError('synthetic failed script')
        if self.option == 'readback-drift': self.path.write_bytes(b'changed during execution')
        if self.option == 'execute-owner-loss': self.original.alive = False
        if self.option == 'execute-deadline': self.original.now += 21
        return b'[ERROR]\n' if self.option == 'badreply' else stdout()


class PointerProbeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.out = Path(self.temp.name) / 'passive-pointer'
        for name, body in [('NATIVE', NATIVE), ('SOURCE', SOURCE), ('EDITOR', EDITOR)]:
            p = patch.object(probe, name, probe.digest(body))
            p.start(); self.addCleanup(p.stop)
        self.api = FakeOriginal()
        self.io = FakeIO(self.api)
        receipt = {'state': 'PASSIVE_EDITOR_V2_SOURCE_AND_NATIVE_COMPILE_HOST_CHECKED_NOT_EXECUTED',
                   'binary': {'bytes': len(NATIVE), 'sha256': probe.digest(NATIVE)[1]},
                   'sourcePinsEqualBeforeAfter': {'tools/diagnostics/native_editor_pointer_snapshot.c':
                       {'bytes': len(SOURCE), 'sha256': probe.digest(SOURCE)[1]}}}
        self.b = {'ownerRunID': 'fixture-owner',
                  'guestCandidate': 'Dev:Tests/pt24g-editor-pointer-fixture-owner/PTEditorPointerV2',
                  'sourcePins': {}}
        for name, path, body in [
            ('nativeSource', '/fixture/source.c', SOURCE),
            ('nativeCandidate', '/fixture/native', NATIVE),
            ('stagedCandidate', str(probe.HARNESS / 'runtime/Dev/Tests/pt24g-editor-pointer-fixture-owner/PTEditorPointerV2'), NATIVE),
            ('editorCandidate', '/fixture/editor', EDITOR),
            ('compileReceipt', '/fixture/compile.json', json.dumps(receipt).encode()),
        ]:
            self.api.inputs[path] = body
            self.b[name] = ref(path, body)
        self.b['sourcePins']['fixture'] = self.b['nativeSource']
        stage_body = json.dumps({'stagedCandidate': self.b['stagedCandidate']}).encode()
        self.api.inputs['/fixture/stage'] = stage_body
        self.b['namespaceStageRef'] = ref('/fixture/stage', stage_body)
        p = patch('editor_probe_namespace.validate_stage',
                  side_effect=lambda b, pin: json.loads(self.api.pinned(pin)))
        p.start(); self.addCleanup(p.stop)
        self.carrier = probe.EditorPointerProbe(self.b, self.api, io=self.io)

    def run_probe(self, deadline=200.):
        return asyncio.run(self.carrier.run(self.out, deadline))

    def failed(self, option, executions=0, error=RuntimeError, deadline=200.):
        self.io.option = option
        with self.assertRaises(error): self.run_probe(deadline)
        result = json.loads((self.out / 'result.json').read_text())
        self.assertEqual(result['state'], 'PASSIVE_EDITOR_PROBE_FAILED_RETAINED')
        self.assertEqual(result['readbackAttempts'], 1)
        self.assertEqual(result['executionAttempts'], executions)
        events = list(self.api.events)
        with self.assertRaisesRegex(RuntimeError, 'Never replay'): self.run_probe()
        self.assertEqual(events, self.api.events)

    def test_exact_readback_precedes_one_execution_and_records_non_authority(self):
        result = self.run_probe()
        self.assertEqual([e[0] for e in self.api.events], ['readback', 'execute'])
        self.assertEqual(result['screen']['mouse_x'], 219)
        self.assertIsNone(result['window']['title'])
        self.assertFalse(result['inputAuthorized']); self.assertFalse(result['hotspotMappingAccepted'])
        self.assertFalse(result['retryAllowed'])
        self.assertTrue(result['lifecyclePending'])
        self.assertEqual(self.io.path.parent, self.out)

    def test_wrong_guest_bytes_never_execute(self): self.failed('wrongbytes')
    def test_failed_readback_never_replays_or_executes(self): self.failed('readback-error')
    def test_owner_loss_after_readback_never_executes(self): self.failed('owner-loss')
    def test_pin_change_after_readback_never_executes(self): self.failed('pin-change')
    def test_aggregate_twenty_second_deadline_is_not_extended(self): self.failed('aggregate-deadline')
    def test_shorter_parent_deadline_is_not_extended(self): self.failed('parent-deadline', deadline=101.)
    def test_script_failure_never_replays(self): self.failed('execute-error', 1)
    def test_malformed_native_reply_refuses(self): self.failed('badreply', 1, ValueError)
    def test_readback_evidence_drift_during_execution_refuses(self): self.failed('readback-drift', 1)
    def test_owner_loss_after_execution_retains_failure(self): self.failed('execute-owner-loss', 1)
    def test_execution_crossing_aggregate_deadline_refuses(self): self.failed('execute-deadline', 1)
    def test_cancellation_is_retained_and_not_replayed(self): self.failed('cancel', error=asyncio.CancelledError)

    def test_real_async_timeout_retains_partial_evidence_without_replay(self):
        self.api.clock = time.monotonic
        self.failed('timeout', error=TimeoutError, deadline=time.monotonic() + .02)

    def test_existing_evidence_directory_refuses_without_dispatch(self):
        self.out.mkdir()
        with self.assertRaises(RuntimeError): self.run_probe()
        self.assertEqual(self.api.events, [])

    def test_wrong_guest_or_host_namespace_refuses_without_dispatch(self):
        for key in ('guestCandidate', 'stagedCandidate'):
            with self.subTest(key=key):
                b = copy.deepcopy(self.b)
                if key == 'guestCandidate': b[key] = 'Dev:PTEditorPointerV2'
                else: b[key]['path'] = '/other/runtime/Dev/Tests/pt24g-editor-pointer-fixture-owner/PTEditorPointerV2'
                carrier = probe.EditorPointerProbe(b, self.api, io=self.io)
                with self.assertRaises(RuntimeError): asyncio.run(carrier.run(self.out, 200.))
        self.assertEqual(self.api.events, [])

    def test_changed_compile_association_or_editor_refuses_before_dispatch(self):
        for key in ('compileReceipt', 'editorCandidate', 'stagedCandidate'):
            with self.subTest(key=key):
                pin = self.b[key]
                body = self.api.inputs[pin['path']]
                altered = b'wrong' if key != 'compileReceipt' else body.replace(b'NOT_EXECUTED', b'EXECUTED')
                self.api.inputs[pin['path']] = altered
                self.b[key] = ref(pin['path'], altered)
                carrier = probe.EditorPointerProbe(self.b, self.api, io=self.io)
                with self.assertRaises(RuntimeError): asyncio.run(carrier.run(self.out, 200.))
                self.api.inputs[pin['path']] = body; self.b[key] = pin
        self.assertEqual(self.api.events, [])

    def test_nonfinite_and_expired_deadlines_never_dispatch(self):
        for value in (True, float('nan'), float('inf'), 100.):
            carrier = probe.EditorPointerProbe(self.b, self.api, io=self.io)
            with self.assertRaises(RuntimeError): asyncio.run(carrier.run(self.out, value))
        self.assertEqual(self.api.events, [])

    def test_caller_binding_mutation_cannot_retarget_a_constructed_probe(self):
        self.b['guestCandidate'] = 'Dev:unrelated'
        self.b['sourcePins'].clear()
        result = self.run_probe()
        self.assertEqual(result['state'], 'PASSIVE_EDITOR_V2_DATA_RECORDED')
        self.assertTrue(all(e[1].endswith('/PTEditorPointerV2') for e in self.api.events))


class OriginalRouteTests(unittest.TestCase):
    """Replace imported transport modules entirely; no network-capable object."""
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.out = Path(self.temp.name)
        self.api = FakeOriginal()
        self.route = probe.OriginalFrameProbeIO(self.api, 'fixture-owner')
        self.fail_init_owner = False
        self.reply_error = False
        self.calls = []
        outer = self

        @asynccontextmanager
        async def transport(url):
            outer.calls.append(('transport', url))
            yield 'fixture-reader', 'fixture-writer', None

        class Session:
            def __init__(self, rd, wr): outer.calls.append(('session', rd, wr))
            async def __aenter__(self): return self
            async def __aexit__(self, *args): return False
            async def initialize(self):
                outer.calls.append(('initialize',))
                if outer.fail_init_owner: outer.api.alive = False
            async def call_tool(self, name, args):
                outer.calls.append(('tool', name, args))
                return SimpleNamespace(model_dump=lambda **kw: {'synthetic': True})

        def require_reply(reply):
            outer.calls.append(('require-reply',))
            if outer.reply_error: raise RuntimeError('synthetic STILL RUNNING refusal')
            return stdout().decode()

        mcp = ModuleType('mcp'); mcp.ClientSession = Session
        client = ModuleType('mcp.client')
        stream = ModuleType('mcp.client.streamable_http'); stream.streamable_http_client = transport
        bridge = ModuleType('bridge_checks'); bridge.require_reply = require_reply
        p = patch.dict(sys.modules, {'mcp': mcp, 'mcp.client': client,
                                    'mcp.client.streamable_http': stream, 'bridge_checks': bridge})
        p.start(); self.addCleanup(p.stop)

    def test_original_script_route_arguments_reply_guard_and_raw_evidence(self):
        body = asyncio.run(self.route.snapshot('Dev:fixture/PTEditorPointerV2', self.out))
        self.assertEqual(body, stdout())
        self.assertIn(('tool', 'amiga_run_script',
                       {'script': 'Dev:fixture/PTEditorPointerV2', 'timeout': 10}), self.calls)
        self.assertEqual(sum(c[0] == 'tool' for c in self.calls), 1)
        self.assertEqual(self.calls[-1], ('require-reply',))
        self.assertEqual(json.loads((self.out/'passive-snapshot-reply.json').read_text()),
                         {'reply': {'synthetic': True}})

    def test_owner_loss_during_initialize_prevents_tool_dispatch(self):
        self.fail_init_owner = True
        with self.assertRaises(RuntimeError): asyncio.run(self.route.snapshot('Dev:fixture', self.out))
        self.assertFalse(any(c[0] == 'tool' for c in self.calls))

    def test_transport_failure_reply_is_saved_then_original_guard_refuses(self):
        self.reply_error = True
        with self.assertRaises(RuntimeError): asyncio.run(self.route.snapshot('Dev:fixture', self.out))
        self.assertTrue((self.out/'passive-snapshot-reply.json').is_file())
        self.assertEqual(sum(c[0] == 'tool' for c in self.calls), 1)

    def test_readback_uses_original_carrier_and_checks_owner_on_return(self):
        async def readback(guest, path):
            self.calls.append(('original-readback', guest, str(path)))
            self.api.alive = False
        self.api.readback = readback
        with self.assertRaises(RuntimeError):
            asyncio.run(self.route.readback('Dev:fixture', self.out/'readback.bin'))
        self.assertEqual(len(self.calls), 1)


if __name__ == '__main__': unittest.main()
