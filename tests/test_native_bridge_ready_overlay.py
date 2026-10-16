"""HOST-only startup ordering/refusal tests; no process, socket or target call."""
import ast
import asyncio
import copy
from pathlib import Path
import sys
import types
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import native_bridge_ready_overlay as overlay

SOURCE = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra/scripts/emulator.py')


class Context:
    def __init__(self, value): self.value = value
    async def __aenter__(self): return self.value
    async def __aexit__(self, *unused): return False


class StartupOrder(unittest.TestCase):
    def setUp(self):
        self.raw = SOURCE.read_bytes()
        self.changed = overlay.corrected_tree(self.raw)

    def execute(self, *, target='amiberry-030', no_bridge=False, bridge_failure=False,
                ipc_failure=False, original=False, require_bridge_before_ping=True):
        tree = ast.parse(self.raw) if original else self.changed
        main = next(n for n in tree.body if isinstance(n, ast.AsyncFunctionDef) and n.name == '_main')
        # Execute only the original protocol suffix with supplied in-memory
        # objects. No launch, file operation, decorators or module imports run.
        start = next(i for i, n in enumerate(main.body)
                     if isinstance(n, ast.Expr) and isinstance(n.value, ast.Await))
        fn = ast.AsyncFunctionDef(name='probe', args=ast.arguments(posonlyargs=[],
            args=[ast.arg(arg='a'), ast.arg(arg='owned'), ast.arg(arg='c')], kwonlyargs=[],
            kw_defaults=[], defaults=[]), body=copy.deepcopy(main.body[start:]), decorator_list=[])
        code = compile(ast.fix_missing_locations(ast.Module(body=[fn], type_ignores=[])), '<private-startup-suffix>', 'exec')
        events = []
        ready = False

        async def wait(owned, seconds=60, ipc=False):
            nonlocal ready
            self.assertEqual(owned, (123, 'EXACT_PRIVATE_PROFILE'))
            events.append(('ipc.wait' if ipc else 'bridge.wait', seconds))
            if not ipc:
                if bridge_failure: raise RuntimeError('OWNED_LISTENER_REFUSED')
                ready = True

        async def ping():
            events.append('ipc.ping')
            if require_bridge_before_ping and target == 'amiberry-030' and not no_bridge and not ready:
                raise TimeoutError('EARLY_PING_MODEL')
            return not ipc_failure

        async def sleep(seconds): events.append(('sleep', seconds))

        class Session:
            async def initialize(inner): events.append('session.initialize')
            async def call_tool(inner, name, arguments):
                events.append((name, arguments))
                text = 'Connected via TCP' if name == 'amiga_connect' else 'Amiga alive'
                return types.SimpleNamespace(content=[types.SimpleNamespace(text=text)])

        def http(url):
            self.assertEqual(url, 'http://127.0.0.1:3000/mcp')
            events.append('http'); return Context(('reader', 'writer', None))

        namespace = {'wait_owned_listener': wait, 'streamable_http_client': http,
                     'ClientSession': lambda rd, wr: Context(Session()),
                     'asyncio': types.SimpleNamespace(sleep=sleep), 'print': lambda *args: None}
        exec(code, namespace)
        try:
            asyncio.run(namespace['probe'](types.SimpleNamespace(target=target, no_bridge=no_bridge),
                                          (123, 'EXACT_PRIVATE_PROFILE'), types.SimpleNamespace(ping=ping)))
            error = None
        except (RuntimeError, TimeoutError) as exc:
            error = exc
        return events, error

    def test_pinned_original_exposes_early_ping_in_readiness_model(self):
        events, error = self.execute(original=True)
        self.assertIsInstance(error, TimeoutError)
        self.assertEqual(events, [('ipc.wait', 30), 'ipc.ping'])

    def test_ready_then_single_ping_and_original_connect(self):
        events, error = self.execute()
        self.assertIsNone(error)
        self.assertEqual(events, [('ipc.wait', 30), ('bridge.wait', 60), 'ipc.ping',
            'http', 'session.initialize', ('amiga_connect', {'mode': 'tcp', 'host': '127.0.0.1', 'port': 2345}),
            ('amiga_ping', {})])

    def test_listener_refusal_prevents_ping_and_connection(self):
        events, error = self.execute(bridge_failure=True)
        self.assertEqual(str(error), 'OWNED_LISTENER_REFUSED')
        self.assertEqual(events, [('ipc.wait', 30), ('bridge.wait', 60)])

    def test_failed_ping_never_connects_or_retries(self):
        events, error = self.execute(ipc_failure=True)
        self.assertIsInstance(error, RuntimeError)
        self.assertEqual(events, [('ipc.wait', 30), ('bridge.wait', 60), 'ipc.ping'])

    def test_stock_and_no_bridge_keep_original_suffix_behavior(self):
        for target, no_bridge in (('amiberry-stock', False), ('amiberry-030', True)):
            with self.subTest(target=target, no_bridge=no_bridge):
                changed = self.execute(target=target, no_bridge=no_bridge)
                original = self.execute(target=target, no_bridge=no_bridge, original=True)
                self.assertEqual(changed, original)
                self.assertEqual(changed, ([('ipc.wait', 30), 'ipc.ping', ('sleep', 20)], None))

    def test_source_change_refused_and_other_functions_identical(self):
        with self.assertRaises(ValueError): overlay.corrected_tree(self.raw + b'\n')
        original = ast.parse(self.raw)
        unchanged = lambda t: [overlay.dump(n) for n in t.body
                               if not isinstance(n, ast.AsyncFunctionDef) or n.name != '_main']
        self.assertEqual(unchanged(original), unchanged(self.changed))


if __name__ == '__main__': unittest.main()
