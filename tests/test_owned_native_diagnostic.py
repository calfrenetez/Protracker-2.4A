"""Named diagnostic collection over the existing failure-sticky transaction."""
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from owned_native_run import Plan, Transaction
import cia_aperture_capture as c
from test_cia_aperture_capture import report
from test_owned_native_run import Bridge


class DiagnosticTransaction(unittest.IsolatedAsyncioTestCase):
    def new(self, data, code):
        self.plan = Plan("d1" * 16, 789, b"HOST MODEL NOT HUNK" * 32, c.MARKERS,
                         262144, observation_profile=c.PROFILE)
        self.bridge = Bridge(self.plan, native_output=data)
        self.bridge.native_rc = code
        self.events = []
        return Transaction(self.plan, self.bridge.exchange, self.events.append)

    async def test_closed_measurement_failure_retains_rc20_and_requires_separate_idle(self):
        for partial, post in ((False, False), (False, True), (True, False)):
            transaction = self.new(report(failed=True, partial=partial, post=post), 20)
            await transaction.prepare()
            result = await transaction.execute()
            self.assertEqual(result["observed"]["returnCode"], 20)
            self.assertEqual(result["observed"]["diagnostic"]["qualification"], "FAILED")
            self.assertEqual(self.bridge.launches, 1)
            self.assertEqual(transaction.custody["native.log"], self.bridge.native_output)
            self.assertEqual(transaction.phase, "COMPLETED")
            self.assertFalse(any(x["type"] == "DELETEFILE" for x in self.bridge.calls))
            async def idle(plan, manifest):
                self.assertEqual(plan, self.plan.description())
                self.assertEqual(manifest, transaction.manifest())
                return True
            await transaction.settle(idle)
            self.assertEqual(transaction.phase, "SETTLED")
            self.assertEqual(self.bridge.files, {"RAM:original": b"preserve", "T:ab_script_1": b"old script"})

    async def test_collection_success_and_measurement_success_remain_distinct(self):
        transaction = self.new(report(), 0)
        await transaction.prepare()
        result = await transaction.execute()
        self.assertEqual(result["observed"]["diagnostic"]["qualification"], "PASSED")
        self.assertEqual(result["plan"]["observationProfile"], c.PROFILE)
        self.assertFalse(any(x["type"] == "DELETEFILE" for x in self.bridge.calls))
        with self.assertRaises(ValueError):
            self.plan.expected_output

    async def test_bad_trace_closure_or_return_code_is_sticky_without_cleanup_or_replay(self):
        for data, code in ((report(), 20), (report(failed=True), 0),
                           (report(failed=True).replace(b"timer_stopped=1", b"timer_stopped=0"), 20),
                           (report() + b"CIA APERTURE HOLD\n", 0), (b"X" * 32769, 20),
                           (report(failed=True), 5)):
            with self.subTest(code=code, size=len(data)):
                transaction = self.new(data, code)
                await transaction.prepare()
                with self.assertRaises(ValueError):
                    await transaction.execute()
                self.assertEqual(transaction.phase, "FAILED")
                self.assertIsNone(transaction.custody)
                self.assertEqual(self.bridge.launches, 1)
                self.assertFalse(any(x["type"] == "DELETEFILE" for x in self.bridge.calls))
                previous = list(self.bridge.calls)
                for followup in (transaction.execute(), transaction.settle(lambda *_: True)):
                    with self.assertRaises(ValueError):
                        await followup
                self.assertEqual(self.bridge.calls, previous)

    async def test_every_exchange_failure_stops_dispatch_retains_files_and_does_not_replay(self):
        transaction = self.new(report(failed=True), 20)
        await transaction.prepare()
        await transaction.execute()
        async def idle(*_):
            return True
        await transaction.settle(idle)
        total = len(self.bridge.calls)
        for fail in range(1, total + 1):
            with self.subTest(exchange=fail):
                transaction = self.new(report(failed=True), 20)
                self.bridge.fail_at = fail
                with self.assertRaises(TimeoutError):
                    await transaction.prepare()
                    await transaction.execute()
                    await transaction.settle(idle)
                self.assertEqual(len(self.bridge.calls), fail)
                previous = list(self.bridge.calls)
                with self.assertRaises(ValueError):
                    await transaction.execute()
                self.assertEqual(self.bridge.calls, previous)
                self.assertEqual(transaction.phase, "FAILED")

    def test_explicit_named_profile_cannot_change_ordinary_acceptance(self):
        for profile in ("", "other", True, 1):
            with self.subTest(profile=profile), self.assertRaises(ValueError):
                Plan("d1" * 16, 1, b"data", c.MARKERS, observation_profile=profile)
        for markers, progress in ((c.MARKERS[:1], False), (c.MARKERS, True), (c.MARKERS[::-1], False)):
            with self.subTest(markers=markers, progress=progress), self.assertRaises(ValueError):
                Plan("d1" * 16, 1, b"data", markers, progress_sidecar=progress, observation_profile=c.PROFILE)
        ordinary = Plan("d1" * 16, 1, b"data", ("PASS",))
        self.assertEqual(ordinary.expected_output, b"PASS\n")
        self.assertEqual(ordinary.output_limit, 8192)
        self.assertNotIn("observationProfile", ordinary.description())


if __name__ == "__main__":
    unittest.main()
