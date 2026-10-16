"""Host models for explicit external-transfer admission, never FTP or hardware."""
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


class TransferredInputTests(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        self.forbidden = ExitStack()
        for name in ('subprocess.Popen', 'socket.socket.connect', 'socket.socket.connect_ex', 'os.kill'):
            self.forbidden.enter_context(patch(name, side_effect=AssertionError('Host model only')))
        self.addCleanup(self.forbidden.close)

    def new(self, **options):
        self.execution = Plan('c4' * 16, 100, bytes(range(256)) * 1282,
                              ('external transfer fixture PASS',), 1048576,
                              copy_source='RAM:PT24G-' + 'c4' * 8 + '-in/NativeTest')
        self.plan = InputPlan(self.execution, None, None, external_transfer=True)
        self.bridge = InputBridge(self.plan)
        self.bridge.files.pop(None)
        self.bridge.directories.add(self.plan.directory)
        self.bridge.files[self.execution.copy_source] = self.execution.binary
        self.events, self.proofs = [], []
        self.stage = InputStage(self.plan, self.bridge.exchange, self.events.append, **options)
        self.transaction = Transaction(self.execution, self.bridge.exchange, self.events.append)

    async def prove_transfer(self, description, manifest):
        self.assertEqual(description, self.plan.description())
        self.assertEqual(manifest, self.stage.manifest())
        self.proofs.append((description, manifest))
        self.assertEqual(self.bridge.calls, [])
        return True

    async def execution_idle(self, *_):
        return self.transaction.phase == 'SETTLING'

    async def input_idle(self, *_):
        return self.transaction.phase == 'SETTLED'

    async def test_verified_transfer_checks_content_without_writes_and_settles_after_execution(self):
        self.new()
        preserved = {k: v for k, v in self.bridge.files.items() if k != self.execution.copy_source}
        ready = await self.stage.prepare_transferred(self.prove_transfer)
        self.assertEqual(ready['plan']['transfer'], 'external')
        self.assertEqual(len(self.proofs), 1)
        self.assertEqual([c['type'] for c in self.bridge.calls], ['LISTDIR', 'CHECKSUM'])
        await self.transaction.prepare()
        await self.transaction.execute()
        await self.transaction.settle(self.execution_idle)
        await self.stage.settle(self.input_idle)
        self.assertEqual((self.stage.phase, self.transaction.phase), ('SETTLED', 'SETTLED'))
        self.assertEqual(self.bridge.files, preserved)
        self.assertEqual(self.bridge.directories, {'RAM:', 'T:'})
        self.assertEqual((self.bridge.input_launches, self.bridge.copy_launches, self.bridge.launches), (0, 1, 1))

    async def test_matching_file_alone_never_admits_or_overwrites(self):
        for proof in (None, False, 1, {}, 'approved'):
            self.new()
            before = dict(self.bridge.files)
            async def verify(*_): return proof
            with self.assertRaises(ValueError):
                await self.stage.prepare_transferred(None if proof is None else verify)
            self.assertEqual(self.bridge.calls, [])
            self.assertEqual(self.bridge.files, before)
            self.assertEqual(self.stage.phase, 'FAILED')
            with self.assertRaises(ValueError):
                await self.stage.prepare_transferred(self.prove_transfer)

    async def test_ordinary_upload_and_copy_modes_cannot_implicitly_adopt(self):
        self.new()
        with self.assertRaisesRegex(ValueError, 'ownership verification'):
            await self.stage.prepare()
        self.assertEqual(self.bridge.calls, [])
        for plan in (InputPlan(self.execution, None, None),
                     InputPlan(self.execution, 'Dev:Tests/PT24G-' + 'c4' * 8 + '/Fixture', 99)):
            stage = InputStage(plan, self.bridge.exchange, self.events.append)
            with self.assertRaisesRegex(ValueError, 'external-transfer plan'):
                await stage.prepare_transferred(self.prove_transfer)
            self.assertEqual(stage.phase, 'FAILED')
        self.assertEqual(self.bridge.calls, [])

    async def test_wrong_missing_or_unowned_contents_retain_every_file(self):
        for fault in ('missing', 'body', 'file', 'directory'):
            self.new()
            if fault == 'missing': self.bridge.files.pop(self.execution.copy_source)
            if fault == 'body':
                self.bridge.files[self.execution.copy_source] = b'X' + self.execution.binary[1:]
            if fault == 'file': self.bridge.files[self.plan.directory + '/unowned'] = b'preserve'
            if fault == 'directory': self.bridge.directories.add(self.plan.directory + '/unowned')
            before = dict(self.bridge.files)
            with self.assertRaises(ValueError):
                await self.stage.prepare_transferred(self.prove_transfer)
            self.assertEqual(self.stage.phase, 'FAILED')
            self.assertEqual(self.bridge.files, before)
            self.assertTrue(all(c['type'] in ('LISTDIR', 'CHECKSUM') for c in self.bridge.calls))

    async def test_each_admission_exchange_failure_stops_without_retry_or_cleanup(self):
        self.new()
        await self.stage.prepare_transferred(self.prove_transfer)
        count = len(self.bridge.calls)
        self.assertEqual(count, 2)
        for index in range(1, count + 1):
            self.new()
            self.bridge.fail_at = index
            before = dict(self.bridge.files)
            with self.assertRaises(TimeoutError):
                await self.stage.prepare_transferred(self.prove_transfer)
            self.assertEqual(len(self.bridge.calls), index)
            self.assertEqual(self.stage.phase, 'FAILED')
            self.assertEqual(self.bridge.files, before)
            with self.assertRaises(ValueError): await self.stage.prepare()
            self.assertEqual(len(self.bridge.calls), index)

    async def test_cancelled_or_raising_ownership_observer_retains_input(self):
        for error in (asyncio.CancelledError(), OSError('receipt unavailable')):
            self.new()
            async def verify(*_): raise error
            with self.assertRaises(type(error)):
                await self.stage.prepare_transferred(verify)
            self.assertEqual(self.stage.phase, 'FAILED')
            self.assertEqual(self.bridge.calls, [])
            self.assertIn(self.execution.copy_source, self.bridge.files)

    async def test_expired_preparation_never_calls_ownership_observer(self):
        now = [0.0]
        self.new(clock=lambda: now[0])
        now[0] = 60
        with self.assertRaisesRegex(ValueError, 'deadline'):
            await self.stage.prepare_transferred(self.prove_transfer)
        self.assertEqual((self.proofs, self.bridge.calls), ([], []))

    async def test_late_or_timed_out_receipt_cannot_start_content_checks(self):
        for timeout in (False, True):
            now = [0.0]
            self.new(clock=lambda: now[0], seconds=.02)
            async def verify(*_):
                if timeout: await asyncio.sleep(1)
                now[0] = .02
                return True
            with self.assertRaises((ValueError, TimeoutError)):
                await self.stage.prepare_transferred(verify)
            self.assertEqual(self.bridge.calls, [])
            self.assertEqual(self.stage.phase, 'FAILED')

    async def test_ready_transfer_does_not_waive_reader_settlement(self):
        self.new()
        await self.stage.prepare_transferred(self.prove_transfer)
        calls = list(self.bridge.calls)
        with self.assertRaisesRegex(ValueError, 'execution-idle'):
            await self.stage.settle(self.input_idle)
        self.assertEqual(self.bridge.calls, calls)
        self.assertIn(self.execution.copy_source, self.bridge.files)

    async def test_content_check_expiry_keeps_original_deadline_and_input(self):
        now = [0.0]
        self.new(clock=lambda: now[0], seconds=200)
        exchange = self.bridge.exchange
        async def delayed(command, *args):
            raw = await exchange(command, *args)
            now[0] = 60
            return raw
        self.stage.exchange = delayed
        with self.assertRaisesRegex(ValueError, 'deadline'):
            await self.stage.prepare_transferred(self.prove_transfer)
        self.assertEqual((self.stage.prepare_deadline, self.stage.deadline), (60, 200))
        self.assertEqual(len(self.bridge.calls), 1)
        self.assertEqual(self.stage.phase, 'FAILED')
        self.assertIn(self.execution.copy_source, self.bridge.files)

    async def test_receipt_intent_record_failure_precedes_any_observer_or_transport(self):
        self.new()
        def failed_record(_): raise OSError('cannot retain transfer intent')
        self.stage.record = failed_record
        with self.assertRaisesRegex(OSError, 'transfer intent'):
            await self.stage.prepare_transferred(self.prove_transfer)
        self.assertEqual((self.proofs, self.bridge.calls), ([], []))
        self.assertEqual(self.stage.phase, 'FAILED')

    def test_external_mode_cannot_name_other_sources_or_expand_bounds(self):
        self.new()
        for flag in (1, 'external', None):
            with self.assertRaises(ValueError): InputPlan(self.execution, None, None, flag)
        with self.assertRaisesRegex(ValueError, 'Dev source'):
            InputPlan(self.execution, 'Dev:Tests/PT24G-' + 'c4' * 8 + '/Fixture', 99, True)
        with self.assertRaisesRegex(ValueError, 'command identity'):
            InputPlan(self.execution, None, 99, True)
        with self.assertRaises(ValueError):
            InputStage(self.plan, self.bridge.exchange, self.events.append, preparation_seconds=121)


if __name__ == '__main__': unittest.main()
