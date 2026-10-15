"""New named capture traverses the existing failure-sticky transaction."""
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from owned_native_run import Plan, Transaction
import cia_aperture_capture as old
import cia_prepared_capture as c
from test_cia_prepared_capture import report
from test_owned_native_run import Bridge


class PreparedDiagnosticTransaction(unittest.IsolatedAsyncioTestCase):
    def new(self, data, rc):
        plan = Plan('d2'*16, 812, b'HOST ONLY NOT HUNK'*32, c.MARKERS, 262144,
                    observation_profile=c.PROFILE)
        bridge = Bridge(plan, native_output=data)
        bridge.native_rc = rc
        events = []
        return Transaction(plan, bridge.exchange, events.append), bridge, events

    async def test_qualification_and_irreversible_failed_measurements_survive_settlement(self):
        for options, rc in (({}, 0), ({'failed': True}, 20), ({'post': True}, 20), ({'long': True}, 20)):
            tx, bridge, events = self.new(report(**options), rc)
            await tx.prepare()
            result = await tx.execute()
            self.assertEqual(result['observed']['returnCode'], rc)
            self.assertEqual(result['observed']['diagnostic']['qualification'], 'PASSED' if rc == 0 else 'FAILED')
            self.assertEqual(tx.custody['native.log'], bridge.native_output)
            self.assertEqual(bridge.launches, 1)
            self.assertFalse(any(x['type'] == 'DELETEFILE' for x in bridge.calls))
            async def idle(plan, manifest):
                self.assertEqual(plan, tx.plan.description())
                self.assertEqual(manifest, tx.manifest())
                return True
            await tx.settle(idle)
            self.assertEqual(tx.phase, 'SETTLED')
            self.assertEqual(bridge.files, {'RAM:original': b'preserve', 'T:ab_script_1': b'old script'})

    async def test_bad_partial_reentry_closure_or_rc_never_dispatches_cleanup_or_replay(self):
        for data, rc in ((report()[:-10], 0), (report(failed=True), 0), (report(), 20),
                         (report(), 5), (report().replace(b'reentry=0', b'reentry=1', 1), 0),
                         (report().replace(b'timer_stopped=1', b'timer_stopped=0'), 0),
                         (b'X'*65537, 20)):
            tx, bridge, _ = self.new(data, rc)
            await tx.prepare()
            with self.assertRaises(ValueError):
                await tx.execute()
            self.assertEqual(tx.phase, 'FAILED')
            self.assertIsNone(tx.custody)
            self.assertFalse(any(x['type'] == 'DELETEFILE' for x in bridge.calls))
            calls = list(bridge.calls)
            with self.assertRaises(ValueError):
                await tx.execute()
            self.assertEqual(bridge.calls, calls)

    def test_profiles_cannot_borrow_each_others_markers_or_relax_ordinary_contract(self):
        for profile, markers in ((old.PROFILE, c.MARKERS), (c.PROFILE, old.MARKERS)):
            with self.assertRaises(ValueError):
                Plan('d2'*16, 1, b'host', markers, observation_profile=profile)
        for progress in (False, True):
            if progress:
                with self.assertRaises(ValueError):
                    Plan('d2'*16, 1, b'host', c.MARKERS, observation_profile=c.PROFILE, progress_sidecar=True)
            else:
                plan = Plan('d2'*16, 1, b'host', c.MARKERS, observation_profile=c.PROFILE)
                self.assertEqual(plan.output_limit, 65536)
                with self.assertRaises(ValueError):
                    plan.expected_output
        plan = Plan('d2'*16, 1, b'host', ('PASS',))
        self.assertEqual(plan.output_limit, 8192)
        self.assertEqual(plan.expected_output, b'PASS\n')


if __name__ == '__main__':
    unittest.main()
