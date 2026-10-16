"""Bounded saved observations, independent of exact scheduling acceptance.

Only sixteen complete pairs with reported clock/task/memory restoration are
collectable. RC0 means observation complete, never a scheduling PASS. Invalid
clock observations retain RC20/FAILED_CLOCK_OBSERVATION. No target operation.
"""
import re
import zlib
from cia_aperture_capture import controls, decimal, fields, require

PROFILE = 'clock-pair-v1'
MAX_OUTPUT = 8192
MODEL = 'CLOCK PAIR MODEL PASS: full-width saved counters, unchanged invalid outputs, rate and regression accounting; observation only'
COMPLETE = 'CLOCK PAIR COMPLETE: sixteen adjacent direct clock observations; no scheduling, IRQ WCET, audio or hardware timing acceptance'
MARKERS = (MODEL, COMPLETE)
FIELDS = frozenset('index first second status delta'.split())
CLOSED = frozenset('attempted completed valid failures task_identity priority unchanged signals_restored clock_closed interrupt_nesting_restored task_nesting_restored'.split())


def words(value, count):
    parts = value.split(':')
    require(len(parts) == count, 'Actual clock observation word count differs')
    return [decimal(part) for part in parts]


def validate(data, return_code):
    require(type(data) is bytes and 0 < len(data) <= MAX_OUTPUT and type(return_code) is int
            and return_code in (0, 20), 'Invalid bounded clock-pair capture')
    try:
        text = data.decode('ascii')
    except UnicodeDecodeError as error:
        raise ValueError('Clock-pair capture is not ASCII') from error
    require(text.endswith('\n') and '\r' not in text and '\x00' not in text, 'Incomplete/noncanonical clock-pair report')
    lines = text.splitlines()
    require(len(lines) == (21 if return_code == 0 else 20) and lines[0] == MODEL, 'Incomplete clock-pair report')
    memory = re.fullmatch(r'EXEC MEMORY pool_limit=(0|[1-9][0-9]{0,9}) reserve=262144 flags=4', lines[1])
    require(memory is not None and decimal(memory[1]) > 0, 'Fast pool declaration differs')
    rows, valid, deltas = [], 0, []
    for i in range(16):
        raw = fields(lines[i+2], 'CLOCK PAIR OBS ', FIELDS)
        row = {'index': decimal(raw['index']), 'first': words(raw['first'], 3),
               'second': words(raw['second'], 3), 'status': decimal(raw['status']), 'delta': words(raw['delta'], 2)}
        require(row['index'] == i, 'Clock-pair order changed')
        a, b = row['first'], row['second']
        first, second = (a[0] << 32) | a[1], (b[0] << 32) | b[1]
        status = 1 if not a[2] or a[2] != b[2] else (2 if second < first else 0)
        delta = second-first if status == 0 else 0
        require(row['status'] == status and ((row['delta'][0] << 32) | row['delta'][1]) == delta,
                'Native clock analysis differs from full-width saved observations')
        accepted = status == 0 and a[2] in (709379, 715909)
        valid += accepted
        if accepted:
            deltas.append(delta)
        rows.append(row)
    raw = fields(lines[18], 'CLOCK PAIR CLOSED ', CLOSED)
    require(re.fullmatch(r'-?(0|[1-9][0-9]{0,2})', raw['priority']) is not None, 'Native priority malformed')
    closed = {k: int(v) if k == 'priority' else decimal(v) for k, v in raw.items()}
    require(-128 <= closed['priority'] <= 127 and closed['attempted'] == closed['completed'] == 16
            and (closed['valid'], closed['failures']) == (valid, 16-valid), 'Clock observation tally differs')
    require(all(closed[k] == 1 for k in ('task_identity', 'unchanged', 'signals_restored', 'clock_closed',
                'interrupt_nesting_restored', 'task_nesting_restored')), 'Reported native closure incomplete')
    require(lines[19] == 'EXEC MEMORY PASS: 1 Fast allocations, zero owned bytes, budget refusal without Chip fallback', 'Fast ownership closure missing')
    complete = valid == 16
    require((return_code == 0) == complete and (not complete or lines[20] == COMPLETE), 'Actual RC and observation completion disagree')
    return {'diagnosticProfile': PROFILE, 'qualification': 'OBSERVATION_COMPLETE' if complete else 'FAILED_CLOCK_OBSERVATION',
            'returnCode': return_code, 'resourceClosureReported': True, 'summary': closed, 'observations': rows,
            'observedDeltaMin': min(deltas) if deltas else None, 'observedDeltaMax': max(deltas) if deltas else None,
            'outputBytes': len(data), 'outputCRC32': f'{zlib.crc32(data):08X}',
            'scope': 'Finite task-only library sampling intervals including minimal intervening instructions; no isolated call duration, universal minimum, WCET, scheduled activation, IRQ, physical timing or listening acceptance'}
