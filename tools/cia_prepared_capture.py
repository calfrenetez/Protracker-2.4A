"""Bounded collector for the separate immutable-descriptor clock experiment.

Only a complete, closed 16-attempt report is collectable here. Partial output,
HOLD, reentry or unknown restoration stays failure-sticky in the transaction.
RC20 timing failure stays FAILED; raw samples and irreversible effects survive.
This module performs no resource, process, transport or target operation.
"""
import re
import zlib
from cia_aperture_capture import controls, decimal, fields, require, CLOSED, SHADOW

PROFILE = 'cia-prepared-aperture-v1'
MAX_OUTPUT = 65536
MODEL = 'CIA PREPARED MODEL PASS: serialized exact-key admission, original windows, full-width recorded-clock replay and irreversible commit accounting; diagnostic only'
PASS = 'CIA PREPARED PASS: original-frame direct-clock RAM brackets and serialized zero-effect negative gates; diagnostic only, no DMA or audio proof'
MARKERS = (MODEL, PASS)
WIDE = frozenset('first last arm_target arm_observed arm_after'.split())
FIELDS = frozenset('index mode phase admitted arm_valid arm_after_valid delivered replay closed count gate result reads commits shadow before_index after_index reentry wake calls busy arm_frequency samples'.split()) | WIDE


def counter(value):
    words = value.split(':')
    require(len(words) == 2, 'Actual native counter needs both words')
    return (decimal(words[0]) << 32) | decimal(words[1])


def replay(row, frequency):
    """Independent integer replay of recorded reads under the private contract.

    This does not authenticate the native key or execution, and cannot replay
    the time of an asynchronous reentry latch. Such reports remain sticky.
    """
    samples = row['samples']
    if row['gate'] == 3:
        require(not samples, 'Refused serialized admission performed a read')
        return (3, 0, 0, 0, 0)
    arm, previous, post = row['first'] - 128, None, False
    before = after = commits = shadow = 0
    for index, (now, rate) in enumerate(samples, 1):
        if post:
            after = index
        else:
            before = index
        if rate != frequency or (previous is not None and now < previous):
            result = 4
        elif now < arm:
            result = 6 if index == 1 else 4
        elif post:
            result = 1 if row['first'] <= now < row['last'] else 9
        elif now >= row['last']:
            result = 5
        elif now >= row['first']:
            commits, shadow, post = 1, SHADOW, True
            previous = now
            continue
        elif index >= 63:
            result = 7
        else:
            previous = now
            continue
        require(index == len(samples), 'Report appends reads after terminal decision')
        return result, commits, shadow, before, after
    raise ValueError('Actual terminal clock sample absent')


def observation(line, index):
    raw = fields(line, 'CIA PREPARED OBS ', FIELDS)
    row = {}
    for name, value in raw.items():
        if name in WIDE:
            row[name] = counter(value)
        elif name == 'arm_frequency':
            words = value.split('/')
            require(len(words) == 2, 'Arm rates malformed')
            row[name] = [decimal(word) for word in words]
        elif name == 'samples':
            entries = value.split(',') if value else []
            require(len(entries) <= 64, 'Native read cap exceeded')
            values = []
            for entry in entries:
                words = entry.split(':')
                require(len(words) == 3, 'Raw read needs both counter words and rate')
                values.append([(decimal(words[0]) << 32) | decimal(words[1]), decimal(words[2])])
            row[name] = values
        else:
            row[name] = decimal(value)
    require(row['index'] == index and row['mode'] == (0 if index < 12 else index - 11), 'Original attempt order changed')
    require(row['phase'] == 6 and all(row[k] == 1 for k in ('arm_valid', 'arm_after_valid', 'delivered', 'replay', 'closed'))
            and not row['reentry'] and not row['busy'] and row['calls'] == index + 1 and row['wake'] > 0,
            'Partial, reentrant or unresolved native observation')
    expected_gate = 3 if index in (12, 13, 15) else 0
    require(row['admitted'] == row['gate'] == expected_gate and row['reads'] == len(row['samples'])
            and row['commits'] <= 1 and row['shadow'] == (SHADOW if row['commits'] else 0), 'Admission/read/commit accounting differs')
    require(0 < row['count'] <= 65535 and row['arm_target'] >= row['arm_observed']
            and row['arm_target'] - row['arm_observed'] == row['count'], 'Actual arm/count relation differs')
    return row


def validate(data, return_code):
    require(isinstance(data, bytes) and 0 < len(data) <= MAX_OUTPUT and type(return_code) is int
            and return_code in (0, 20), 'Invalid bounded prepared capture')
    try:
        text = data.decode('ascii')
    except UnicodeDecodeError as error:
        raise ValueError('Prepared capture is not ASCII') from error
    require(text.endswith('\n') and '\r' not in text and '\x00' not in text, 'Incomplete/noncanonical prepared report')
    lines = text.splitlines()
    require(len(lines) == (21 if return_code == 0 else 20) and lines[0] == MODEL, 'Incomplete prepared report or wrong completion marker')
    memory = re.fullmatch(r'EXEC MEMORY pool_limit=(0|[1-9][0-9]{0,9}) reserve=262144 flags=4', lines[1])
    require(memory is not None and decimal(memory[1]) > 0, 'Fast pool declaration differs')
    rows = [observation(lines[i+2], i) for i in range(16)]
    raw = fields(lines[18], 'CIA PREPARED CLOSED ', CLOSED)
    closed = {k: int(v) if k == 'priority' and re.fullmatch(r'-?[0-9]{1,3}', v) else decimal(v) for k, v in raw.items()}
    require(-128 <= closed['priority'] <= 127 and all(closed[k] == 1 for k in
            ('acquired', 'vector_removed', 'timer_stopped', 'task_identity', 'unchanged', 'signals_restored'))
            and closed['chip'] <= 1 and closed['bit'] <= 1, 'Reported resource restoration incomplete')
    require(closed['attempted'] == closed['completed'] == 16 and closed['frequency'] in (709379, 715909)
            and (closed['early_ticks'], closed['residency_ticks'], closed['reads_cap']) == (128, 256, 64), 'Fixed experiment policy changed')
    frequency, epoch, positive, negative = closed['frequency'], None, 0, 0
    for row in rows:
        frame = (row['index']+1)*1536
        current_epoch = row['first'] - (frame*frequency+47999)//48000
        if epoch is None:
            epoch = current_epoch
        require(epoch >= 0 and current_epoch == epoch and
                row['last'] == epoch + ((frame+1)*frequency+47999)//48000, 'Original frame window or epoch changed')
        require(row['arm_target'] == (row['last']+128 if row['mode'] == 3 else row['first']-128)
                and row['arm_frequency'] == [frequency, frequency], 'Original arm target/rate changed')
        expected = replay(row, frequency)
        require(tuple(row[k] for k in ('result', 'commits', 'shadow', 'before_index', 'after_index')) == expected,
                'Native decision differs from complete actual-clock replay')
        if row['mode'] == 0:
            passed = row['result'] == 1 and row['commits'] == 1 and 2 <= row['reads'] <= 64
            if passed:
                require(0 <= row['samples'][-1][0] - row['samples'][0][0] <= 256, 'Actual aperture span exceeds residency cap')
            positive += passed
        else:
            negative += row['result'] == (5 if row['mode'] == 3 else 3) and row['commits'] == 0
    require((positive, negative, 16-positive-negative) == (closed['positive'], closed['negative'], closed['failures']), 'Outcome tally differs')
    require(lines[19] == 'EXEC MEMORY PASS: 2 Fast allocations, zero owned bytes, budget refusal without Chip fallback', 'Fast ownership closure missing')
    passed = (positive, negative) == (12, 4)
    require((return_code == 0) == passed and (not passed or lines[20] == PASS), 'Actual RC and original success gates disagree')
    return {'diagnosticProfile': PROFILE, 'qualification': 'PASSED' if passed else 'FAILED',
            'returnCode': return_code, 'resourceClosureReported': True, 'summary': closed,
            'observations': rows, 'originalEpoch': epoch, 'outputBytes': len(data),
            'outputCRC32': f'{zlib.crc32(data):08X}',
            'scope': 'Saved complete timer/RAM diagnostic only; no authenticated resource closure, real sample ownership, total IRQ WCET, output, hardware timing or listening acceptance'}
