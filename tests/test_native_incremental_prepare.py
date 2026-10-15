"""Host fault injection for one live transaction's bounded upload calls."""
import asyncio
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from owned_native_run import Plan, Transaction
from test_owned_native_run import Bridge


class IncrementalPreparationTests(unittest.IsolatedAsyncioTestCase):
    def new(self, size=166396, **kwargs):
        self.plan = Plan('b2' * 16, 456789, (bytes(range(256)) * ((size + 255) // 256))[:size],
                         ('native first PASS', 'native final PASS'), stack=262144)
        self.bridge = Bridge(self.plan)
        self.records = []
        return Transaction(self.plan, self.bridge.exchange, self.records.append, **kwargs)

    async def prepare(self, run):
        progress = await run.begin_prepare()
        while progress['phase'] == 'UPLOADING':
            before = len(self.bridge.calls)
            old_offset = progress['nextOffset']
            progress = await run.prepare_chunk(old_offset)
            sent = self.bridge.calls[before:]
            self.assertTrue(1 <= len(sent) <= 4)
            self.assertTrue(all(c['type'] == 'WRITEFILE' for c in sent))
            self.assertEqual(sent[0]['offset'], old_offset)
            self.assertGreater(progress['nextOffset'], old_offset)
        self.assertEqual(progress['phase'], 'UPLOADED')
        self.assertEqual(progress['nextOffset'], len(self.plan.binary))
        return await run.finish_prepare()

    async def test_full_candidate_with_bounded_calls_then_existing_execution_and_settlement(self):
        now = [0.0]
        run = self.new(seconds=140, clock=lambda: now[0])
        async def slow_write(command, kind, seconds):
            if command['type'] == 'WRITEFILE':
                now[0] += 1.7
            return await self.bridge.exchange(command, kind, seconds)
        run.exchange = slow_write
        description = await self.prepare(run)
        self.assertEqual(description, self.plan.description())
        self.assertEqual(run.deadline, 140)
        self.assertGreater(now[0], 60)  # Synthetic total, not a wall-time/hardware measurement.
        self.assertEqual(self.bridge.files[self.plan.path('NativeTest')], self.plan.binary)
        self.assertEqual(self.bridge.launches, 0)
        candidate_writes = [c for c in self.bridge.calls if c['type'] == 'WRITEFILE'
                            and c['path'] == self.plan.path('NativeTest')]
        self.assertEqual(len(candidate_writes), 41)
        result = await run.execute()
        self.assertEqual(result['observed']['returnCode'], 0)
        self.assertEqual(self.bridge.launches, 1)
        async def idle(description, manifest):
            return description == self.plan.description() and len(manifest) == 5
        await run.settle(idle)
        self.assertEqual(self.bridge.files, {'RAM:original': b'preserve', 'T:ab_script_1': b'old script'})
        self.assertEqual(run.deadline, 140)

    async def test_short_final_chunks_and_large_plan_reconstruct_exactly(self):
        for size in (1, 4067, 4068, 4069, 16272, 16273, 424732):
            with self.subTest(size=size):
                run = self.new(size)
                await self.prepare(run)
                self.assertEqual(self.bridge.files[self.plan.path('NativeTest')], self.plan.binary)

    async def test_every_preparation_exchange_failure_retains_files_and_forbids_resume(self):
        run = self.new(20000)
        await self.prepare(run)
        count = len(self.bridge.calls)
        for fail_at in range(1, count + 1):
            with self.subTest(exchange=fail_at):
                run = self.new(20000)
                self.bridge.fail_at = fail_at
                with self.assertRaises(TimeoutError):
                    await self.prepare(run)
                self.assertEqual(run.phase, 'FAILED')
                stopped = len(self.bridge.calls)
                for op in (run.begin_prepare, lambda: run.prepare_chunk(0), run.finish_prepare,
                           run.prepare, run.execute):
                    with self.assertRaises(ValueError):
                        await op()
                self.assertEqual(len(self.bridge.calls), stopped)
                self.assertFalse(any(c['type'] in ('LAUNCH', 'DELETEFILE', 'SCRIPT') for c in self.bridge.calls))
                self.assertEqual(self.bridge.files['RAM:original'], b'preserve')

    async def test_stale_or_invalid_offset_cannot_replay_a_successful_chunk(self):
        for offset in (0, -1, True, 16272.0, '16272', 999999):
            with self.subTest(offset=offset):
                run = self.new()
                await run.begin_prepare()
                await run.prepare_chunk(0)
                count = len(self.bridge.calls)
                with self.assertRaisesRegex(ValueError, 'Exact next upload offset'):
                    await run.prepare_chunk(offset)
                self.assertEqual(run.phase, 'FAILED')
                self.assertEqual(len(self.bridge.calls), count)

    async def test_no_early_finish_or_execute_or_monolithic_restart(self):
        run = self.new()
        await run.begin_prepare()
        await run.prepare_chunk(0)
        count = len(self.bridge.calls)
        for op in (run.begin_prepare, run.finish_prepare, run.prepare, run.execute):
            with self.assertRaises(ValueError):
                await op()
        self.assertEqual(len(self.bridge.calls), count)
        self.assertEqual(self.bridge.launches, 0)

    async def test_deadline_includes_time_between_calls(self):
        now = [0.0]
        run = self.new(seconds=140, clock=lambda: now[0])
        await run.begin_prepare()
        progress = await run.prepare_chunk(0)
        count = len(self.bridge.calls)
        now[0] = 140.0
        with self.assertRaisesRegex(ValueError, 'Whole transaction deadline'):
            await run.prepare_chunk(progress['nextOffset'])
        self.assertEqual(run.deadline, 140)
        self.assertEqual(run.phase, 'FAILED')
        self.assertEqual(len(self.bridge.calls), count)

    async def test_cancellation_in_each_frame_never_replays_or_cleans(self):
        for frame in range(1, 5):
            with self.subTest(frame=frame):
                run = self.new()
                await run.begin_prepare()
                writes = [0]
                async def cancel(command, kind, seconds):
                    writes[0] += 1
                    if writes[0] == frame:
                        raise asyncio.CancelledError()
                    return await self.bridge.exchange(command, kind, seconds)
                run.exchange = cancel
                with self.assertRaises(asyncio.CancelledError):
                    await run.prepare_chunk(0)
                self.assertEqual(run.phase, 'FAILED')
                count = len(self.bridge.calls)
                with self.assertRaises(ValueError):
                    await run.prepare_chunk(0)
                self.assertEqual(len(self.bridge.calls), count)
                self.assertFalse(any(c['type'] in ('DELETEFILE', 'LAUNCH') for c in self.bridge.calls))

    async def test_wrong_ack_and_progress_record_failure_stop(self):
        for fault in ('short_ack', 'progress_record'):
            with self.subTest(fault=fault):
                run = self.new()
                await run.begin_prepare()
                if fault == 'short_ack':
                    self.bridge.change_reply = lambda c, r: 'OK|WRITEFILE|1' if c['type'] == 'WRITEFILE' else r
                else:
                    def record(value):
                        if value['event'] == 'upload_progress':
                            raise OSError('durable progress record failed')
                        self.records.append(value)
                    run.record = record
                with self.assertRaises((ValueError, OSError)):
                    await run.prepare_chunk(0)
                self.assertEqual(run.phase, 'FAILED')
                self.assertEqual(self.bridge.launches, 0)

    async def test_changed_uploaded_candidate_stops_before_script_or_launch(self):
        run = self.new(4000)
        await run.begin_prepare()
        await run.prepare_chunk(0)
        self.bridge.files[self.plan.path('NativeTest')] = b'changed'
        with self.assertRaisesRegex(ValueError, 'Guest bytes/CRC differ'):
            await run.finish_prepare()
        self.assertEqual(run.phase, 'FAILED')
        self.assertNotIn(self.plan.path('Run-once'), self.bridge.files)
        self.assertEqual(self.bridge.launches, 0)

    async def test_new_transaction_never_adopts_partial_namespace(self):
        run = self.new()
        await run.begin_prepare()
        await run.prepare_chunk(0)
        files = dict(self.bridge.files)
        other = Transaction(self.plan, self.bridge.exchange, self.records.append)
        with self.assertRaisesRegex(ValueError, 'Fresh directory creation unconfirmed'):
            await other.begin_prepare()
        self.assertEqual(self.bridge.files, files)
        self.assertEqual(other.phase, 'FAILED')

    async def test_emulator_copy_route_cannot_enter_incremental_upload(self):
        self.new()
        plan = Plan(self.plan.run_id, self.plan.command_id, self.plan.binary, self.plan.markers,
                    copy_source='Dev:Tests/PT24G-' + self.plan.run_id[:16] + '/Fixture')
        run = Transaction(plan, self.bridge.exchange, self.records.append)
        with self.assertRaisesRegex(ValueError, 'requires direct upload'):
            await run.begin_prepare()
        self.assertEqual(self.bridge.calls, [])


if __name__ == '__main__':
    unittest.main()
