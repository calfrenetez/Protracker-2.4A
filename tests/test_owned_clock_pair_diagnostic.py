"""Explicit non-acceptance observation over unchanged failure-sticky custody."""
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from owned_native_run import Plan, Transaction
import clock_pair_capture as c
import cia_aperture_capture as aperture
import cia_prepared_capture as prepared
from test_clock_pair_capture import report
from test_owned_native_run import Bridge


class ClockPairTransaction(unittest.IsolatedAsyncioTestCase):
    def new(self, data, rc):
        plan = Plan('e5'*16, 852, b'HOST ONLY NOT HUNK'*32, c.MARKERS, 262144, observation_profile=c.PROFILE)
        bridge = Bridge(plan, native_output=data);bridge.native_rc = rc
        return Transaction(plan, bridge.exchange, lambda event: None), bridge

    async def test_complete_and_failed_clock_observation_retain_original_custody_and_rc(self):
        for failed in (False, True):
            rc = 20 if failed else 0
            tx, bridge = self.new(report(failed), rc)
            await tx.prepare();result = await tx.execute()
            self.assertEqual(result['observed']['returnCode'], rc)
            self.assertEqual(result['observed']['diagnostic']['qualification'], 'FAILED_CLOCK_OBSERVATION' if failed else 'OBSERVATION_COMPLETE')
            self.assertEqual(tx.custody['native.log'], bridge.native_output)
            self.assertEqual(bridge.launches, 1)
            async def idle(plan, manifest):
                return plan == tx.plan.description() and manifest == tx.manifest()
            await tx.settle(idle)
            self.assertEqual(tx.phase, 'SETTLED')
            self.assertEqual(bridge.files, {'RAM:original': b'preserve', 'T:ab_script_1': b'old script'})

    async def test_partial_or_uncertain_closure_never_replays_or_cleans(self):
        for data, rc in ((report()[:-1], 0), (report(), 20), (report(True), 0), (report(), 5),
                         (report().replace(b'clock_closed=1', b'clock_closed=0'), 0)):
            tx, bridge = self.new(data, rc);await tx.prepare()
            with self.assertRaises(ValueError):
                await tx.execute()
            self.assertEqual(tx.phase, 'FAILED');self.assertIsNone(tx.custody)
            self.assertFalse(any(x['type'] == 'DELETEFILE' for x in bridge.calls))
            calls = list(bridge.calls)
            with self.assertRaises(ValueError):
                await tx.execute()
            self.assertEqual(bridge.calls, calls)

    def test_profiles_and_sidecars_cannot_borrow_the_observation_contract(self):
        for other in (aperture, prepared):
            for profile, markers in ((other.PROFILE, c.MARKERS), (c.PROFILE, other.MARKERS)):
                with self.assertRaises(ValueError):
                    Plan('e5'*16, 1, b'host', markers, observation_profile=profile)
        with self.assertRaises(ValueError):
            Plan('e5'*16, 1, b'host', c.MARKERS, observation_profile=c.PROFILE, progress_sidecar=True)
        plan = Plan('e5'*16, 1, b'host', c.MARKERS, observation_profile=c.PROFILE)
        self.assertEqual(plan.output_limit, 8192)
        with self.assertRaises(ValueError):
            plan.expected_output
        ordinary = Plan('e5'*16, 1, b'host', ('PASS',))
        self.assertEqual(ordinary.expected_output, b'PASS\n')
