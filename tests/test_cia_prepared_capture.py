"""Synthetic complete reports exercise the new collection boundary only."""
from pathlib import Path
import json
import re
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import cia_prepared_capture as c


def report(*, frequency=709379, failed=False, post=False, long=False):
    lines = [c.MODEL, 'EXEC MEMORY pool_limit=10000000 reserve=262144 flags=4']
    epoch = (1 << 48) - 35000
    for i in range(16):
        first = epoch + ((i+1)*1536*frequency+47999)//48000
        last = epoch + (((i+1)*1536+1)*frequency+47999)//48000
        mode = 0 if i < 12 else i-11
        gate = 3 if i in (12, 13, 15) else 0
        samples = [] if gate else ([[last+128, frequency]] if i == 14 else
                   ([[first-1, frequency]]*63 if long else
                    ([[first-1, frequency], [last+1, frequency]] if failed else
                     [[first-24, frequency], [first, frequency], [last if post else first+1, frequency]])))
        row = dict(index=i, mode=mode, phase=6, admitted=gate, arm_valid=1,
                   arm_after_valid=1, delivered=1, replay=1, closed=1, first=first, last=last,
                   arm_target=last+128 if mode == 3 else first-128,
                   arm_frequency=[frequency, frequency], count=100, gate=gate,
                   reads=len(samples), reentry=0, wake=2, calls=i+1, busy=0, samples=samples)
        row['arm_observed'] = row['arm_target']-100
        row['arm_after'] = row['arm_observed']+20
        values = c.replay(row, frequency)
        row.update(zip(('result', 'commits', 'shadow', 'before_index', 'after_index'), values))
        fields = []
        for k, v in row.items():
            if k in c.WIDE:
                value = f'{v>>32}:{v&0xffffffff}'
            elif k == 'samples':
                value = ','.join(f'{t>>32}:{t&0xffffffff}:{rate}' for t, rate in v)
            elif k == 'arm_frequency':
                value = '/'.join(str(x) for x in v)
            else:
                value = str(v)
            fields.append(k+'='+value)
        lines.append('CIA PREPARED OBS '+' '.join(fields))
    positive = 0 if failed or post or long else 12
    lines += [f'CIA PREPARED CLOSED acquired=1 chip=0 bit=1 vector_removed=1 timer_stopped=1 task_identity=1 priority=0 unchanged=1 signals_restored=1 attempted=16 completed=16 positive={positive} negative=4 failures={12-positive} frequency={frequency} early_ticks=128 residency_ticks=256 reads_cap=64',
              'EXEC MEMORY PASS: 2 Fast allocations, zero owned bytes, budget refusal without Chip fallback']
    if positive == 12:
        lines.append(c.PASS)
    return ('\n'.join(lines)+'\n').encode('ascii')


class PreparedCapture(unittest.TestCase):
    def test_actual_full_word_read_lists_and_json_roundtrip(self):
        for frequency in (709379, 715909):
            result = c.validate(report(frequency=frequency), 0)
            self.assertEqual(result['qualification'], 'PASSED')
            self.assertEqual(json.loads(json.dumps(result)), result)
            self.assertEqual(len(result['observations']), 16)
            self.assertEqual(result['observations'][12]['samples'], [])
            self.assertEqual(result['originalEpoch'], (1 << 48)-35000)

    def test_failed_before_after_and_read_budget_remain_failed(self):
        for options in ({'failed': True}, {'post': True}, {'long': True}):
            result = c.validate(report(**options), 20)
            self.assertEqual(result['qualification'], 'FAILED')
            self.assertEqual(result['returnCode'], 20)
            self.assertEqual(result['summary']['positive'], 0)
            self.assertEqual(result['observations'][0]['commits'], int(options.get('post', False)))
        with self.assertRaises(ValueError):
            c.validate(report(failed=True), 0)
        with self.assertRaises(ValueError):
            c.validate(report(), 20)

    def test_no_rebase_unknown_rc_partial_hold_or_unverified_restoration(self):
        raw = report()
        bad = [raw.replace(b'vector_removed=1', b'vector_removed=0'),
               raw.replace(b'reentry=0', b'reentry=1', 1),
               raw.replace(b'closed=1', b'closed=0', 1),
               raw.replace(b'replay=1', b'replay=0', 1),
               raw.replace(b'calls=1 ', b'calls=2 ', 1),
               raw.replace(b'index=1 ', b'index=0 ', 1),
               raw.replace(b'count=100 ', b'count=101 ', 1),
               raw.replace(b'shadow=1095779666', b'shadow=0', 1),
               raw.replace(b'result=1 ', b'result=5 ', 1),
               raw.replace(b'reads=3 ', b'reads=2 ', 1),
               raw.replace(b'before_index=2 ', b'before_index=1 ', 1),
               raw.replace(b'early_ticks=128', b'early_ticks=129'),
               raw.replace(b'positive=12', b'positive=11'),
               raw.replace(b'CIA PREPARED OBS index=15', b'CIA PREPARED HOLD index=15'),
               b'X'*65537, raw+b'HOLD\n', raw.split(b'CIA PREPARED OBS index=15')[0]]
        # Change only one original window, preserving all raw readings.
        match = re.search(rb'first=([0-9]+):([0-9]+)', raw)
        bad.append(raw[:match.start(2)]+str(int(match[2])+1).encode()+raw[match.end(2):])
        for value in bad:
            with self.subTest(value=value[:80]), self.assertRaises(ValueError):
                c.validate(value, 0)
        for rc in (1, 5, True, '0'):
            with self.subTest(rc=rc), self.assertRaises(ValueError):
                c.validate(raw, rc)

    def test_saved_schema_matches_native_printf(self):
        native = (Path(__file__).resolve().parent/'native_exec_cia_prepared_aperture_test.c').read_text()
        line = next(x for x in native.splitlines() if 'printf("CIA PREPARED OBS ' in x)
        self.assertEqual(set(re.findall(r'([a-z_]+)=', line)), c.FIELDS)


if __name__ == '__main__':
    unittest.main()
