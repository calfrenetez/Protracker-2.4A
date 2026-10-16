from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import clock_pair_capture as c


def report(failed=False):
    lines = [c.MODEL, 'EXEC MEMORY pool_limit=33554432 reserve=262144 flags=4']
    for i in range(16):
        # One pair genuinely crosses the low-word boundary. Another can fail
        # without losing its actual words or being treated as wrapped elapsed.
        first = (9 << 32) + 0xfffffff0 + i*4096
        second = first + 12 + i
        if failed and i == 7:
            second = first-1
        status = 2 if second < first else 0
        delta = 0 if status else second-first
        lines.append(f'CLOCK PAIR OBS index={i} first={first>>32}:{first&0xffffffff}:709379 '
                     f'second={second>>32}:{second&0xffffffff}:709379 status={status} delta={delta>>32}:{delta&0xffffffff}')
    lines.append(f'CLOCK PAIR CLOSED attempted=16 completed=16 valid={15 if failed else 16} failures={int(failed)} '
                 'task_identity=1 priority=0 unchanged=1 signals_restored=1 clock_closed=1 '
                 'interrupt_nesting_restored=1 task_nesting_restored=1')
    lines.append('EXEC MEMORY PASS: 1 Fast allocations, zero owned bytes, budget refusal without Chip fallback')
    if not failed:
        lines.append(c.COMPLETE)
    return ('\n'.join(lines)+'\n').encode('ascii')


class ClockPairCapture(unittest.TestCase):
    def test_observation_is_not_scheduling_acceptance_and_survives_json(self):
        import json
        result = c.validate(report(), 0)
        self.assertEqual(result['qualification'], 'OBSERVATION_COMPLETE')
        self.assertEqual((result['observedDeltaMin'], result['observedDeltaMax']), (12, 27))
        self.assertEqual(json.loads(json.dumps(result)), result)

    def test_genuine_failed_clock_preserves_words_and_rc(self):
        result = c.validate(report(True), 20)
        self.assertEqual(result['qualification'], 'FAILED_CLOCK_OBSERVATION')
        self.assertEqual(result['returnCode'], 20)
        self.assertEqual(result['observations'][7]['status'], 2)
        self.assertEqual(result['observations'][7]['delta'], [0, 0])

    def test_rate_failure_precedes_regression_and_remains_failed(self):
        data = report(True).replace(b'index=7 first=10:28656:709379', b'index=7 first=10:28656:0').replace(b'status=2', b'status=1')
        self.assertIn(b'first=10:28656:0', data)
        self.assertEqual(c.validate(data, 20)['observations'][7]['status'], 1)

    def test_partial_extra_bad_word_or_report_order_is_sticky(self):
        original = report()
        for bad in (original[:-1], original+b'EXTRA\n', original.replace(b'index=7 ', b'index=8 ', 1),
                    original.replace(b'index=7 ', b'index=07 ', 1), original.replace(b'delta=0:12', b'delta=0:13', 1),
                    original.replace(b'first=9:', b'first=4294967296:', 1), original.replace(b'status=0', b'status=1', 1),
                    original.replace(b'\n', b'\r\n'), b'X'*8193, original.replace(b'CLOCK PAIR OBS', b'CLOCK PAIR HOLD', 1)):
            with self.subTest(bad=bad[:100]), self.assertRaises(ValueError):
                c.validate(bad, 0)

    def test_closure_and_original_rc_are_mandatory(self):
        for bad in (report().replace(b'clock_closed=1', b'clock_closed=0'),
                    report().replace(b'signals_restored=1', b'signals_restored=0'),
                    report().replace(b'interrupt_nesting_restored=1', b'interrupt_nesting_restored=0'),
                    report().replace(b'task_nesting_restored=1', b'task_nesting_restored=0'),
                    report().replace(b'attempted=16', b'attempted=15'),
                    report().replace(b'valid=16', b'valid=15'), report().replace(b'priority=0', b'priority=128')):
            with self.subTest(bad=bad[-400:]), self.assertRaises(ValueError):
                c.validate(bad, 0)
        for data, rc in ((report(), 20), (report(True), 0), (report(), 5), (report(), True)):
            with self.assertRaises(ValueError):
                c.validate(data, rc)


if __name__ == '__main__':
    unittest.main()
