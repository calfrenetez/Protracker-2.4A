"""Single-file physical staging model; no native, socket or process effects."""
import asyncio
from contextlib import ExitStack
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from owned_native_input import InputPlan, InputStage
from owned_native_run import Plan, Transaction
from test_owned_native_input import InputBridge


class UploadTests(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        self.forbidden = ExitStack()
        for name in ('subprocess.Popen', 'socket.socket.connect', 'socket.socket.connect_ex', 'os.kill'):
            self.forbidden.enter_context(patch(name, side_effect=AssertionError('Host model only')))
        self.addCleanup(self.forbidden.close)

    def new(self, size=224436, **options):
        self.execution = Plan('b3' * 16, 100, (bytes(range(256)) * (size // 256 + 1))[:size],
                              ('new first PASS', 'new final PASS'), 262144,
                              copy_source='RAM:PT24G-' + 'b3' * 8 + '-in/NativeTest')
        self.plan = InputPlan(self.execution, None, None)
        self.bridge = InputBridge(self.plan)
        self.bridge.files.pop(None)  # Upload has neither a Dev nor an existing RAM source.
        self.events = []
        self.stage = InputStage(self.plan, self.bridge.exchange, self.events.append, **options)
        self.transaction = Transaction(self.execution, self.bridge.exchange, self.events.append)
        return self.stage

    async def input_idle(self, plan, manifest):
        self.assertEqual(plan, self.plan.description())
        self.assertEqual(set(manifest), {'NativeTest'})
        return self.transaction.phase == 'SETTLED'

    async def execution_idle(self, plan, manifest):
        return True

    async def lifecycle(self):
        await self.stage.prepare()
        await self.transaction.prepare()
        await self.transaction.execute()
        await self.transaction.settle(self.execution_idle)
        await self.stage.settle(self.input_idle)

    async def test_upload_stages_one_file_without_launch_or_source_adoption(self):
        self.new(preparation_seconds=120)
        original = dict(self.bridge.files)
        ready = await self.stage.prepare()
        self.assertEqual(ready['status'], 'INPUT_READY_CUSTODY_RETAINED')
        self.assertEqual(self.plan.names, ('NativeTest',))
        self.assertEqual(self.bridge.files[self.execution.copy_source], self.execution.binary)
        self.assertEqual(self.plan.description()['transfer'], 'upload')
        self.assertFalse(any(c['type'] in ('LAUNCH', 'SCRIPT', 'DELETEFILE') for c in self.bridge.calls))
        self.assertNotIn('command', self.plan.description())
        await self.transaction.prepare()
        await self.transaction.execute()
        await self.transaction.settle(self.execution_idle)
        await self.stage.settle(self.input_idle)
        self.assertEqual(self.bridge.files, original)
        self.assertEqual(self.bridge.directories, {'RAM:', 'T:'})
        self.assertEqual((self.bridge.input_launches, self.bridge.copy_launches, self.bridge.launches), (0, 1, 1))

    async def test_each_exchange_failure_retains_source_and_never_replays(self):
        self.new()
        await self.lifecycle()
        count = len(self.bridge.calls)
        for index in range(1, count + 1):
            with self.subTest(exchange=index):
                self.new()
                self.bridge.fail_at = index
                with self.assertRaises(TimeoutError):
                    await self.lifecycle()
                self.assertEqual(len(self.bridge.calls), index)
                self.assertIn('FAILED', (self.stage.phase, self.transaction.phase))
                self.assertEqual(self.bridge.files['RAM:original'], b'preserve')
                self.assertEqual(self.bridge.files['T:ab_script_1'], b'old script')
                with self.assertRaises(ValueError):
                    await self.stage.prepare()
                self.assertEqual(len(self.bridge.calls), index)

    async def test_existing_input_never_adopted_or_overwritten(self):
        self.new()
        self.bridge.directories.add(self.plan.directory)
        self.bridge.files[self.plan.path('NativeTest')] = b'other owner'
        before = dict(self.bridge.files)
        with self.assertRaisesRegex(ValueError, 'Fresh input directory'):
            await self.stage.prepare()
        self.assertEqual(self.bridge.files, before)
        self.assertEqual([c['type'] for c in self.bridge.calls], ['MAKEDIR'])

    async def test_maximum_size_fits_fixed_exchange_bound(self):
        self.new(512 * 1024)
        await self.lifecycle()
        self.assertLess(self.stage.calls, 256)
        with self.assertRaisesRegex(ValueError, 'capacity'):
            self.new(512 * 1024 + 1)

    async def test_bad_copy_identity_and_script_access_refused(self):
        self.new()
        with self.assertRaisesRegex(ValueError, 'no command identity'):
            InputPlan(self.execution, None, 42)
        with self.assertRaisesRegex(ValueError, 'no preparation script'):
            _ = self.plan.script
        with self.assertRaisesRegex(ValueError, 'launches no command'):
            _ = self.plan.command

    async def test_changed_or_extra_source_refuses_settlement(self):
        for fault in ('body', 'file', 'directory'):
            self.new()
            await self.stage.prepare()
            if fault == 'body': self.bridge.files[self.execution.copy_source] = b'changed'
            if fault == 'file': self.bridge.files[self.plan.directory + '/other'] = b'unowned'
            if fault == 'directory': self.bridge.directories.add(self.plan.directory + '/other')
            async def claimed_idle(*args): return True
            before = dict(self.bridge.files)
            with self.assertRaises(ValueError):
                await self.stage.settle(claimed_idle)
            self.assertEqual(self.bridge.files, before)
            self.assertFalse(any(c['type'] == 'DELETEFILE' for c in self.bridge.calls))

    async def test_reader_still_active_or_uncertain_cannot_delete_source(self):
        self.new()
        await self.stage.prepare()
        calls = len(self.bridge.calls)
        with self.assertRaisesRegex(ValueError, 'execution-idle'):
            await self.stage.settle(self.input_idle)
        self.assertEqual(len(self.bridge.calls), calls)
        self.assertIn(self.execution.copy_source, self.bridge.files)

    async def test_separate_preparation_budget_is_explicit_and_never_resets_lifetime(self):
        for limit in (0, -1, 121, True, float('inf'), float('nan')):
            with self.assertRaises(ValueError): self.new(preparation_seconds=limit)
        clock = [1.0]
        self.new(seconds=200, clock=lambda: clock[0], preparation_seconds=120)
        self.assertEqual((self.stage.prepare_deadline, self.stage.deadline), (121, 201))
        exchange = self.bridge.exchange
        async def slow(command, kind, seconds):
            clock[0] += 1.5
            return await exchange(command, kind, seconds)
        self.stage.exchange = slow
        await self.stage.prepare()
        self.assertGreater(clock[0], 60)
        self.assertEqual(self.stage.deadline, 201)
        clock[0] = 202
        calls = len(self.bridge.calls)
        with self.assertRaisesRegex(ValueError, 'deadline'):
            await self.stage.settle(self.input_idle)
        self.assertEqual(len(self.bridge.calls), calls)

    async def test_default_remains_sixty_and_whole_budget_can_only_shorten_it(self):
        self.new(clock=lambda: 10)
        self.assertEqual(self.stage.prepare_deadline, 70)
        self.new(clock=lambda: 10, seconds=20, preparation_seconds=120)
        self.assertEqual((self.stage.prepare_deadline, self.stage.deadline), (30, 30))
        copied = InputPlan(self.execution, 'Dev:Tests/PT24G-' + 'b3' * 8 + '/Fixture', 99)
        with self.assertRaises(ValueError):
            InputStage(copied, self.bridge.exchange, self.events.append, preparation_seconds=61)

    async def test_upload_preparation_expiry_never_launches_or_deletes(self):
        clock = [0]
        self.new(clock=lambda: clock[0], preparation_seconds=120)
        exchange = self.bridge.exchange
        async def slow(command, kind, seconds):
            clock[0] += 3
            return await exchange(command, kind, seconds)
        self.stage.exchange = slow
        with self.assertRaisesRegex(ValueError, 'deadline'):
            await self.stage.prepare()
        self.assertEqual(self.stage.phase, 'FAILED')
        self.assertFalse(any(c['type'] in ('LAUNCH', 'DELETEFILE') for c in self.bridge.calls))
        self.assertLessEqual(clock[0], 120)


if __name__ == '__main__': unittest.main()
