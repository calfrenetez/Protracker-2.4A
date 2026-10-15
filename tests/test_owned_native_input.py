"""Host-only source creation, execution custody and exact cleanup fault tests."""
import asyncio
from contextlib import ExitStack
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from owned_native_input import InputPlan, InputStage
from owned_native_run import Plan, Transaction
from test_owned_native_run import Bridge


class InputBridge(Bridge):
    def __init__(self, plan):
        super().__init__(plan.execution)
        # A ready RAM file is deliberately not pre-seeded. Only the caller's
        # owned Dev source exists before the helper's genuine CreateDir/copy.
        self.files.pop(plan.execution.copy_source)
        self.files[plan.source] = plan.execution.binary
        self.input_plan = plan
        self.input_launches = 0
        self.input_complete = True
        self.input_error = False
        self.input_log_missing = False
        self.mutate_source_on_copy = False

    async def exchange(self, command, kind, seconds):
        plan = self.input_plan
        if command['type'] == 'LISTDIR':
            self.calls.append(dict(command))
            if len(self.calls) == self.fail_at:
                raise TimeoutError('injected input directory failure')
            path = command['path']
            prefix = path if path.endswith(':') else path + '/'
            entries = [(name[len(prefix):], len(body), 'F') for name, body in self.files.items()
                       if name.startswith(prefix) and '/' not in name[len(prefix):]]
            entries += [(name[len(prefix):], 0, 'D') for name in self.directories
                        if name != path and name.startswith(prefix) and '/' not in name[len(prefix):]]
            body = ','.join(f'{name}({size},{typ})' for name, size, typ
                            in sorted(entries)[command['offset']:])
            return self.change_reply(command, f'DIR|{path}|{len(entries)}|{body}')
        if command['type'] == 'LAUNCH' and command['id'] == plan.command_id:
            self.calls.append(dict(command))
            if len(self.calls) == self.fail_at:
                raise TimeoutError('injected input transport failure')
            assert command == {'type': 'LAUNCH', 'id': plan.command_id, 'command': plan.command}
            script = self.files[plan.path('Prepare-input')]
            assert (f'C:Copy {plan.source} {plan.path("NativeTest")} QUIET\n').encode() in script
            assert (f'C:Protect {plan.path("NativeTest")} +rwed\n').encode() in script
            self.input_launches += 1
            self.files[plan.path('NativeTest')] = self.files[plan.source]
            self.successful_script_outputs(script)
            if self.input_error:
                self.files[plan.path('ready.flag')] = b'FAILED\n'
            if not self.input_complete:
                self.files[plan.path('ready.flag')] = b''
            if self.input_log_missing:
                self.files[plan.path('prepare.log')] = b''
            if self.mutate_source_on_copy:
                self.files[plan.source] += b'changed'
            return self.change_reply(command, f'CMD|{plan.command_id}|OK|Started: {plan.command}')
        return await super().exchange(command, kind, seconds)


class InputTests(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        # This suite cannot accidentally start a local service, connect, signal
        # a process or launch an emulator while exercising the actual helpers.
        self.forbidden = ExitStack()
        for name in ('subprocess.Popen', 'socket.socket.connect', 'socket.socket.connect_ex', 'os.kill'):
            self.forbidden.enter_context(patch(name, side_effect=AssertionError('Host model only')))
        self.addCleanup(self.forbidden.close)

    def new(self, **kwargs):
        self.execution = Plan('a2' * 16, 100, bytes(range(256)) * 877,
                              ('publication first PASS', 'publication final PASS'), 262144,
                              copy_source='RAM:PT24G-' + 'a2' * 8 + '-in/NativeTest',
                              progress_sidecar=True)
        self.plan = InputPlan(self.execution, 'Dev:Tests/PT24G-' + 'a2' * 8 + '/Fixture', 99)
        self.bridge = InputBridge(self.plan)
        self.events = []
        self.input = InputStage(self.plan, self.bridge.exchange, self.events.append, **kwargs)
        self.transaction = Transaction(self.execution, self.bridge.exchange, self.events.append, **kwargs)
        self.input_idle_calls = 0
        return self.input

    async def execution_idle(self, plan, manifest):
        self.assertEqual(plan, self.execution.description())
        self.assertEqual(set(manifest), set(self.execution.names))
        self.assertEqual(self.transaction.phase, 'SETTLING')
        return True

    async def input_idle(self, plan, manifest):
        self.input_idle_calls += 1
        self.assertEqual(plan, self.plan.description())
        self.assertEqual(manifest, self.input.manifest())
        return self.transaction.phase == 'SETTLED' and self.execution.directory not in self.bridge.directories

    async def lifecycle(self):
        await self.input.prepare()
        await self.transaction.prepare()
        await self.transaction.execute()
        await self.transaction.settle(self.execution_idle)
        await self.input.settle(self.input_idle)

    async def test_full_source_creation_execution_and_separate_cleanup(self):
        self.new()
        original = dict(self.bridge.files)
        self.assertNotIn(self.execution.copy_source, original)
        self.assertLessEqual(len(self.plan.directory.split(':')[1]), 30)
        await self.lifecycle()
        self.assertEqual((self.input.phase, self.transaction.phase), ('SETTLED', 'SETTLED'))
        self.assertEqual((self.bridge.input_launches, self.bridge.copy_launches, self.bridge.launches), (1, 1, 1))
        self.assertEqual(self.input_idle_calls, 1)
        self.assertEqual(self.bridge.files, original)
        self.assertEqual(self.bridge.directories, {'RAM:', 'T:'})
        self.assertFalse(any(c['type'] in ('SCRIPT', 'UPLOAD') for c in self.bridge.calls))
        self.assertFalse(any(c['type'] != 'CHECKSUM' and c.get('path') == self.plan.source for c in self.bridge.calls))
        self.assertFalse(any(c['type'] == 'WRITEFILE' and c.get('path', '').endswith('/NativeTest')
                             for c in self.bridge.calls))
        input_delete = next(i for i, c in enumerate(self.bridge.calls)
                            if c['type'] == 'DELETEFILE' and c['path'] == self.execution.copy_source)
        execution_delete = next(i for i, c in enumerate(self.bridge.calls)
                                if c['type'] == 'DELETEFILE' and c['path'] == self.execution.directory)
        self.assertGreater(input_delete, execution_delete)

    async def test_each_exchange_failure_never_replays_or_continues(self):
        self.new()
        await self.lifecycle()
        total = len(self.bridge.calls)
        self.assertGreater(total, 100)
        for index in range(1, total + 1):
            with self.subTest(exchange=index):
                self.new()
                self.bridge.fail_at = index
                with self.assertRaises(TimeoutError):
                    await self.lifecycle()
                self.assertEqual(len(self.bridge.calls), index)
                self.assertIn('FAILED', (self.input.phase, self.transaction.phase))
                self.assertEqual(self.bridge.files[self.plan.source], self.execution.binary)
                self.assertEqual(self.bridge.files['RAM:original'], b'preserve')
                self.assertEqual(self.bridge.files['T:ab_script_1'], b'old script')
                with self.assertRaises(ValueError):
                    await self.input.prepare()
                self.assertEqual(len(self.bridge.calls), index)

    async def test_existing_ram_namespace_is_never_adopted(self):
        self.new()
        self.bridge.directories.add(self.plan.directory)
        self.bridge.files[self.execution.copy_source] = self.execution.binary
        before = dict(self.bridge.files)
        with self.assertRaisesRegex(ValueError, 'Fresh input directory'):
            await self.input.prepare()
        self.assertEqual(self.bridge.files, before)
        self.assertEqual([c['type'] for c in self.bridge.calls], ['CHECKSUM', 'MAKEDIR'])

    async def test_bad_source_refuses_before_mutation(self):
        self.new()
        self.bridge.files[self.plan.source] = b'wrong'
        with self.assertRaisesRegex(ValueError, 'checksum/size'):
            await self.input.prepare()
        self.assertEqual(len(self.bridge.calls), 1)
        self.assertNotIn(self.plan.directory, self.bridge.directories)

    async def test_source_mutation_after_copy_retains_both_namespaces(self):
        self.new()
        self.bridge.mutate_source_on_copy = True
        with self.assertRaisesRegex(ValueError, 'checksum/size'):
            await self.input.prepare()
        self.assertEqual(self.input.phase, 'FAILED')
        self.assertEqual(self.bridge.input_launches, 1)
        self.assertEqual(self.bridge.launches, 0)
        self.assertFalse(any(c['type'] == 'DELETEFILE' for c in self.bridge.calls))

    async def test_copy_failure_or_missing_explicit_log_cannot_launch_native(self):
        for fault in ('input_error', 'input_log_missing'):
            with self.subTest(fault=fault):
                self.new()
                setattr(self.bridge, fault, True)
                with self.assertRaises(ValueError):
                    await self.input.prepare()
                self.assertEqual(self.input.phase, 'FAILED')
                self.assertEqual(self.bridge.launches, 0)
                self.assertFalse(any(c['type'] == 'DELETEFILE' for c in self.bridge.calls))

    async def test_input_ready_alone_does_not_allow_cleanup(self):
        self.new()
        await self.input.prepare()
        before = list(self.bridge.calls)
        with self.assertRaisesRegex(ValueError, 'execution-idle'):
            await self.input.settle(self.input_idle)
        self.assertEqual(self.bridge.calls, before)
        self.assertEqual(self.input.phase, 'FAILED')
        self.assertIn(self.execution.copy_source, self.bridge.files)

    async def test_changed_input_or_unowned_member_prevents_any_delete(self):
        for mutation in ('new_file', 'new_directory', 'changed_binary', 'changed_script', 'changed_source'):
            with self.subTest(mutation=mutation):
                self.new()
                await self.input.prepare()
                await self.transaction.prepare()
                await self.transaction.execute()
                await self.transaction.settle(self.execution_idle)
                path = {'new_file': self.plan.directory + '/unowned',
                        'new_directory': self.plan.directory + '/unowned-directory',
                        'changed_binary': self.execution.copy_source,
                        'changed_script': self.plan.path('Prepare-input'),
                        'changed_source': self.plan.source}[mutation]
                if mutation == 'new_directory':
                    self.bridge.directories.add(path)
                else:
                    self.bridge.files[path] = b'changed'
                start = len(self.bridge.calls)
                with self.assertRaises(ValueError):
                    await self.input.settle(self.input_idle)
                self.assertFalse(any(c['type'] == 'DELETEFILE' for c in self.bridge.calls[start:]))
                self.assertEqual(self.input.phase, 'FAILED')

    async def test_failed_record_prevents_dispatch(self):
        self.new()
        def record(event):
            if event['event'] == 'input_dispatch':
                raise OSError('failed durable intent')
        self.input.record = record
        with self.assertRaises(OSError):
            await self.input.prepare()
        self.assertEqual(self.bridge.calls, [])
        self.assertEqual(self.input.phase, 'FAILED')

    async def test_cancellation_retains_input_and_refuses_replay(self):
        self.new()
        exchange = self.input.exchange
        async def cancel(command, *args):
            if command['type'] == 'LAUNCH':
                raise asyncio.CancelledError()
            return await exchange(command, *args)
        self.input.exchange = cancel
        with self.assertRaises(asyncio.CancelledError):
            await self.input.prepare()
        count = len(self.bridge.calls)
        with self.assertRaises(ValueError):
            await self.input.prepare()
        self.assertEqual(len(self.bridge.calls), count)
        self.assertIn(self.plan.directory, self.bridge.directories)

    async def test_time_limits_include_preparation_readers_and_settlement(self):
        now = [0.0]
        self.new(clock=lambda: now[0])
        await self.input.prepare()
        await self.transaction.prepare()
        await self.transaction.execute()
        await self.transaction.settle(self.execution_idle)
        now[0] = 250
        before = list(self.bridge.calls)
        with self.assertRaisesRegex(ValueError, 'deadline'):
            await self.input.settle(self.input_idle)
        self.assertEqual(self.bridge.calls, before)
        self.assertEqual(self.input_idle_calls, 0)

    async def test_timed_out_or_late_settlement_proof_cannot_delete(self):
        for timeout in (False, True):
            with self.subTest(timeout=timeout):
                now = [0.0]
                self.new(clock=lambda: now[0])
                await self.input.prepare()
                before = list(self.bridge.calls)
                now[0] = 249.99
                async def prove(*args):
                    if timeout:
                        await asyncio.sleep(1)
                    now[0] = 250
                    return True
                with self.assertRaises((ValueError, TimeoutError)):
                    await self.input.settle(prove)
                self.assertEqual(self.bridge.calls, before)
                self.assertEqual(self.input.phase, 'FAILED')

    async def test_successful_lifetime_keeps_original_deadline_across_readers(self):
        now = [0.0]
        self.new(clock=lambda: now[0])
        await self.input.prepare()
        now[0] = 50
        await self.transaction.prepare()
        await self.transaction.execute()
        await self.transaction.settle(self.execution_idle)
        now[0] = 140
        await self.input.settle(self.input_idle)
        self.assertEqual((self.input.deadline, self.transaction.deadline), (250, 250))
        self.assertEqual(self.input.phase, 'SETTLED')

    async def test_ambiguous_creation_or_launch_reply_halts_without_followup(self):
        for operation in ('MAKEDIR', 'LAUNCH'):
            with self.subTest(operation=operation):
                self.new()
                def changed(command, reply):
                    return 'OK|unconfirmed' if command['type'] == operation else reply
                self.bridge.change_reply = changed
                with self.assertRaises(ValueError):
                    await self.input.prepare()
                self.assertEqual(self.bridge.calls[-1]['type'], operation)
                self.assertEqual(self.input.phase, 'FAILED')
                self.assertFalse(any(c['type'] == 'DELETEFILE' for c in self.bridge.calls))

    async def test_preparation_has_its_own_sixty_second_ceiling(self):
        now = [0.0]
        self.new(clock=lambda: now[0])
        exchange = self.input.exchange
        async def delayed(command, *args):
            result = await exchange(command, *args)
            now[0] = 60
            return result
        self.input.exchange = delayed
        with self.assertRaisesRegex(ValueError, 'deadline'):
            await self.input.prepare()
        self.assertEqual(len(self.bridge.calls), 1)

    async def test_copy_does_not_widen_thirty_second_poll_deadline(self):
        now = [0.0]
        async def sleep(seconds): now[0] += seconds
        self.new(clock=lambda: now[0], sleep=sleep)
        self.bridge.input_complete = False
        with self.assertRaisesRegex(ValueError, 'completion deadline'):
            await self.input.prepare()
        self.assertEqual(now[0], 30)
        self.assertEqual(self.bridge.input_launches, 1)
        self.assertFalse(any(c['type'] == 'DELETEFILE' for c in self.bridge.calls))

    def test_paths_and_commands_cannot_cross_run_or_collide(self):
        self.new()
        for source in ('RAM:already-there/NativeTest', self.execution.copy_source,
                       self.plan.source.replace('a2', 'a3'), self.plan.source + ';Quit',
                       self.plan.source + '/other'):
            with self.subTest(source=source), self.assertRaises(ValueError):
                InputPlan(self.execution, source, 99)
        for ident in (100, 101, 0, True, 0x100000000):
            with self.subTest(command=ident), self.assertRaises(ValueError):
                InputPlan(self.execution, self.plan.source, ident)


if __name__ == '__main__':
    unittest.main()
