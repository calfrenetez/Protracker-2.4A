"""Use the existing guarded transaction model with the actual trace contract.

This does not exercise a guest, service, filesystem wire implementation or SDK.
Final closure/RC/custody and no follow-up after failed exchanges stay required.
"""
import asyncio
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from owned_native_run import Plan, Transaction
import readers_memory_trace as trace
from test_owned_native_run import Bridge


class ReadersTraceCapture(unittest.IsolatedAsyncioTestCase):
    def fixture(self):
        self.now = 0
        self.events = []
        self.plan = Plan('b2'*16, 123456, b'candidate', trace.markers(),
                         stack=1048576, progress_sidecar=True)
        self.bridge = Bridge(self.plan)
        self.bridge.progress_output = trace.FINAL_SIDECAR
        return Transaction(self.plan, self.bridge.exchange, self.events.append,
                           clock=lambda: self.now, sleep=self.sleep)

    async def sleep(self, seconds):
        self.now += seconds

    async def test_complete_full_trace_and_exact_separate_sidecar_custody(self):
        transaction = self.fixture()
        await transaction.prepare()
        result = await transaction.execute(diagnostic_progress=True)
        self.assertEqual(result['observed']['returnCode'], 0)
        self.assertEqual(transaction.custody['native.log'], self.plan.expected_output)
        self.assertEqual(transaction.custody['progress.log'], trace.FINAL_SIDECAR)
        self.assertEqual(len(transaction.custody), 6)
        self.assertEqual(self.bridge.launches, 1)
        self.assertEqual(transaction.phase, 'COMPLETED')

    async def test_every_trace_line_without_completion_still_fails_original_deadline(self):
        transaction = self.fixture();self.bridge.complete = False
        await transaction.prepare()
        with self.assertRaisesRegex(ValueError, 'Native completion deadline'):
            await transaction.execute(diagnostic_progress=True)
        progress = [x for x in self.events if x['event']=='native_progress_unconfirmed']
        self.assertEqual([x['elapsed'] for x in progress], [20,40,60,80])
        self.assertTrue(all(bytes.fromhex(x['hexData'])==trace.FINAL_SIDECAR for x in progress))
        self.assertEqual((self.now,transaction.phase,transaction.custody), (90,'FAILED',None))
        count = len(self.bridge.calls)
        for operation in (transaction.execute, transaction.prepare):
            with self.assertRaises(ValueError):
                await operation()
        self.assertEqual(len(self.bridge.calls), count)

    async def test_trace_success_does_not_hide_nonzero_native_return(self):
        transaction = self.fixture();self.bridge.native_rc = 20
        await transaction.prepare()
        with self.assertRaisesRegex(ValueError, 'controls'):
            await transaction.execute(diagnostic_progress=True)
        self.assertEqual((transaction.phase,transaction.custody), ('FAILED',None))
        self.assertFalse(any(x['event']=='completed' for x in self.events))


if __name__=='__main__':
    unittest.main()
