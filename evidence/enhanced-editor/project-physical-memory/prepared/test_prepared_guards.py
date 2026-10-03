"""Host synthetic tests ONLY; never import central live APIs or touch locks."""
import asyncio
import ast
import importlib.util
import json
import tempfile
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('prepared_physical', HERE / 'physical_project_stream.py')
p = importlib.util.module_from_spec(spec)
spec.loader.exec_module(p)
PLAN = json.loads((HERE / 'plan.json').read_text())
BINARY = Path(PLAN['binary'])
LOG = (b'EXEC MEMORY pool_limit=130910912 reserve=262144 flags=4\n'
       b'PROJECT STREAM PASS: exact golden layout/CRC, mixed masters/loops/slices/extensions, sink refusal, bounded workspace=7164\n'
       b'EXEC MEMORY PASS: 6 Fast allocations, zero owned bytes, budget refusal without Chip fallback\n')


class Fake:
    def __init__(self, fault=None):
        self.plan, self.fault = PLAN, fault
        self.target, self.calls, self.connections = 'amiberry-030', [], []
        self.clock, self.inventory_calls, self.guards = 0.0, 0, 0

    def guard_original(self, local=False):
        self.guards += 1
        if self.fault == 'original' and self.guards == 4:
            raise RuntimeError('original changed')
        return {'synthetic': True}

    def require_target(self, name):
        p.require(self.target == name, 'wrong target')
        return {'host': name, 'connected': True}

    async def connect(self, name):
        self.connections.append(name)
        if self.fault == 'connect' or (self.fault == 'return' and name == 'amiberry-030'):
            raise RuntimeError('synthetic unknown connection')
        self.target = name

    def checksum(self, text, data):
        if self.fault == 'checksum':
            raise RuntimeError('bad CRC/size')

    async def pause(self, seconds):
        self.clock += seconds

    async def call(self, name, arguments):
        self.calls.append((name, arguments))
        self.clock += 0.1
        if name == 'amiga_checksum':
            return 'synthetic checksum'
        if name == 'amiga_push_file':
            if self.fault == 'upload':
                raise TimeoutError('synthetic upload incomplete')
            return 'synthetic pushed'
        if name == 'amiga_pull_file':
            target = Path(arguments['local_path'])
            data = LOG if arguments['amiga_path'].endswith('/project.log') else BINARY.read_bytes()
            if self.fault == 'readback' and target.name == 'staged-readback.bin':
                data = b'wrong'
            if self.fault == 'log' and target.name == 'project.log':
                data = LOG.replace(b'6 Fast allocations', b'5 Fast allocations')
            target.write_bytes(data)
            return 'synthetic pulled'
        script = arguments['script']
        if 'PTG-PHYSICAL-IDENTITY-DONE' in script:
            return ('68020' if self.fault == 'identity' else '68030') + "\n192.168.0.156 (on interface 'plipbox')\nPTG-PHYSICAL-IDENTITY-DONE\n"
        if 'PTG-FRESH-ABSENT' in script:
            return '' if self.fault == 'existing' else 'PTG-FRESH-ABSENT\n'
        if 'PTG-DIRECTORY-CREATED' in script:
            return 'PTG-DIRECTORY-CREATED\n'
        if 'PTG-FILES-BEGIN' in script:
            self.inventory_calls += 1
            names = [[], [p.NAME], [p.NAME, 'project.log']][self.inventory_calls - 1]
            if self.fault == 'unexpected' and self.inventory_calls == 2:
                names += ['foreign.file']
            if self.fault == 'temporary' and self.inventory_calls == 3:
                names += ['project.ptg.pttmp-unknown-0']
            directories = [p.NAME] if self.fault == 'directory' and self.inventory_calls == 2 else []
            return ('PTG-FILES-BEGIN\n' + ''.join(x + '\n' for x in names) + 'PTG-FILES-END\nPTG-DIRS-BEGIN\n' +
                    ''.join(x + '\n' for x in directories) + 'PTG-DIRS-END\n')
        if 'Stack 65536' in script:
            if self.fault == 'native-timeout':
                raise TimeoutError('synthetic native still running')
            return 'PTG-PROJECT-RC ' + ('20' if self.fault == 'native-fail' else '0') + '\nPTG-PROJECT-DONE\n'
        if 'PTG-FIXTURE-ABSENT' in script:
            return 'PTG-FIXTURE-ABSENT\n'
        if 'PTG-CLEANUP-ABSENT' in script:
            if self.fault == 'cleanup':
                raise TimeoutError('synthetic cleanup unknown')
            return 'PTG-CLEANUP-ABSENT\n'
        if 'PTG-INDEPENDENT-ABSENT-' in script:
            marker = script.split('Echo ')[-1].strip()
            return '' if self.fault == 'absence' and marker.endswith('-3') else marker + '\n'
        raise AssertionError('Unexpected fake call: ' + repr((name, arguments)))


class PreparedGuards(unittest.TestCase):
    def transaction(self, fault=None, seconds=300):
        with tempfile.TemporaryDirectory(prefix='ptg-mock-', dir='/private/tmp') as temp:
            out = Path(temp) / 'physical-project-123456789'
            out.mkdir()
            backend = Fake(fault)
            budget = p.Budget(seconds, lambda: backend.clock)
            result = {'passed': False, 'recovery_hold': False, 'target_switch_attempted': False}
            error = None
            try:
                asyncio.run(p.transaction(out, BINARY, backend, result, budget))
            except BaseException as caught:
                error = caught
            return backend, result, error

    def test_complete_one_case(self):
        backend, result, error = self.transaction()
        self.assertIsNone(error)
        self.assertTrue(result['passed'])
        self.assertFalse(result['recovery_hold'])
        self.assertEqual(result['native_launches'], 1)
        self.assertEqual(backend.connections, ['real-a1200', 'amiberry-030'])
        self.assertEqual(len(result['physical_absence_observations']), 11)
        self.assertGreater(result['physical_absence_seconds'], 10)
        native = [a for n, a in backend.calls if n == 'amiga_run_script' and 'Stack 65536' in a['script']]
        self.assertEqual(len(native), 1)
        self.assertEqual(native[0]['timeout'], 90)
        deletes = [a['script'] for n, a in backend.calls if n == 'amiga_run_script' and 'Delete ' in a['script']]
        self.assertEqual(len(deletes), 1)
        self.assertEqual(deletes[0].count('Delete '), 3)
        self.assertNotIn(' ALL', deletes[0])
        self.assertNotIn('#?', deletes[0])

    def test_preselection_budget_refusal(self):
        backend, result, error = self.transaction(seconds=180)
        self.assertIsNotNone(error)
        self.assertFalse(result['target_switch_attempted'])
        self.assertFalse(result['recovery_hold'])
        self.assertEqual(backend.connections, [])

    def test_all_failures_stop_and_retain_no_retry(self):
        for fault in ['connect', 'identity', 'existing', 'upload', 'checksum', 'readback', 'unexpected', 'directory', 'native-timeout', 'native-fail', 'log', 'temporary', 'cleanup', 'absence', 'return', 'original']:
            with self.subTest(fault=fault):
                backend, result, error = self.transaction(fault)
                self.assertIsNotNone(error)
                self.assertFalse(result['passed'])
                self.assertTrue(result['recovery_hold'])
                self.assertLessEqual(result['native_launches'], 1)
                self.assertEqual(backend.connections.count('real-a1200'), 1)
                if fault != 'return':
                    self.assertNotIn('amiberry-030', backend.connections)
                if fault in ['native-fail', 'native-timeout', 'log', 'temporary']:
                    self.assertFalse(any('Delete ' in a.get('script', '') for _, a in backend.calls))
                if fault == 'native-timeout':
                    self.assertTrue(result['native_running_or_unknown'])
                if fault == 'absence':
                    self.assertEqual(len(result['physical_absence_observations']), 4)
                    self.assertFalse(result['physical_absence_observations'][-1]['completed'])

    def test_inventory_and_marker_negative_controls(self):
        for text in ['PTG-FILES-BEGIN\nextra\nPTG-FILES-END\nPTG-DIRS-BEGIN\nPTG-DIRS-END\n',
                     'PTG-FILES-END\nPTG-FILES-BEGIN\nPTG-DIRS-BEGIN\nPTG-DIRS-END\n',
                     'PTG-FILES-BEGIN\nPTG-FILES-BEGIN\nPTG-FILES-END\nPTG-DIRS-BEGIN\nPTG-DIRS-END\n',
                     'PTG-FILES-BEGIN\nPTG-FILES-END\nPTG-DIRS-BEGIN\nproject.log\nPTG-DIRS-END\n']:
            with self.assertRaises(RuntimeError):
                p.inventory(text, [])
        for bad in [LOG + b'extra\n', LOG.replace(b'7164', b'7165'), LOG.replace(b'zero owned bytes', b'owned bytes unknown')]:
            with self.assertRaises(RuntimeError):
                p.native_log(bad)

    def test_root_gate_exact_scope_and_freshness(self):
        with tempfile.TemporaryDirectory(prefix='ptg-gate-', dir='/private/tmp') as temp:
            path = Path(temp) / 'gate.json'
            now = datetime.now(timezone.utc)
            base = {'scope': p.SCOPE, 'target': 'real-a1200', 'candidate_sha256': p.SHA, 'original_pid': 19081,
                    'no_conflicting_claims': True, 'recovery_holds_clear': True, 'safari_view_only': True,
                    'browser_control_not_acquired': True, 'coordination_take_ack': 'synthetic test only',
                    'observed_utc': now.isoformat()}
            def check(value):
                data = (json.dumps(value) + '\n').encode()
                path.write_bytes(data)
                return p.gate_record(path, p.digest(data), now)
            self.assertEqual(check(base), base)
            for key, value in [('target', 'amiberry-030'), ('candidate_sha256', 'bad'), ('original_pid', 1), ('recovery_holds_clear', False), ('safari_view_only', False), ('coordination_take_ack', ''), ('observed_utc', (now - timedelta(seconds=601)).isoformat()), ('observed_utc', (now + timedelta(seconds=1)).isoformat())]:
                with self.subTest(key=key), self.assertRaises(RuntimeError):
                    check({**base, key: value})

    def test_ast_no_top_level_live_import_or_finally_cleanup(self):
        tree = ast.parse((HERE / 'physical_project_stream.py').read_text())
        top_imports = [node for node in tree.body if isinstance(node, (ast.Import, ast.ImportFrom))]
        names = [node.module if isinstance(node, ast.ImportFrom) else item.name for node in top_imports for item in ([None] if isinstance(node, ast.ImportFrom) else node.names)]
        self.assertFalse(set(names) & {'amiga', 'shared_guest', 'emulator', 'bridge_checks', 'mcp'})
        main = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == 'main')
        final = next(node for node in main.body if isinstance(node, ast.Try)).finalbody
        self.assertFalse(any(isinstance(node, ast.Attribute) and node.attr in ['connect', 'call', 'unlink', 'rmdir'] for statement in final for node in ast.walk(statement)))


if __name__ == '__main__':
    unittest.main(verbosity=2)
