"""Parse the finite blank-editor log prefix; no target or input operations.

This observes a log snapshot, not GUI readiness, process liveness, admission,
audio quiet, or settlement. The owning harness must independently establish
those facts and enforce its finite deadline before and after a capture.
"""
from dataclasses import dataclass
import hashlib
import re

MAX_LOG_BYTES = 16384
MAX_LINE_BYTES = 1024
MAX_HISTORY_BYTES = 16 * 1024 * 1024
MAX_CLEAN_FRAMES = 32
BLANK_FRAME = (b'EDITOR FRAME row=0 channel=0 revision=0 dirty=0 status=READY - '
               b'F8 PLAY / F9 PATTERN / F10 STOP panel=0')
RUN_LABEL = re.compile(r'blank-capture-[a-z0-9][a-z0-9-]{0,47}')


class InvalidBlankLog(ValueError):
    """The snapshot cannot belong to the admitted blank startup sequence."""


@dataclass(frozen=True)
class BlankLogSnapshot:
    state: str
    byte_count: int
    sha256: str
    history_bytes: int | None
    clean_frames: int


def recent_line(run_label):
    if not isinstance(run_label, str) or RUN_LABEL.fullmatch(run_label) is None:
        raise InvalidBlankLog('Fresh bounded blank-capture run label required')
    return ('EDITOR RECENT loaded=0 prefix=Dev:Tests/' + run_label + '/recent').encode('ascii')


def inspect_blank_log(body, run_label, *, expected_history_bytes):
    """Return WAITING or CLEAN_BLANK_PREFIX; reject every unexpected line.

    A partial final line is only accepted as WAITING while it is a possible
    prefix of the next expected record. A previously observed clean frame
    never makes a later partial, unexpected or active line acceptable.
    """
    recent = recent_line(run_label)
    if type(expected_history_bytes) is not int or not 0 < expected_history_bytes <= MAX_HISTORY_BYTES:
        raise InvalidBlankLog('Exact candidate history size must be independently supplied')
    native_ready = (b'EDITOR READY channels=4 patterns=1 bitmap_bytes=163840 history_bytes='
                    + str(expected_history_bytes).encode('ascii'))
    if not isinstance(body, bytes) or len(body) > MAX_LOG_BYTES:
        raise InvalidBlankLog('Bounded byte log snapshot required')
    if any(v not in (10,) and not 32 <= v <= 126 for v in body):
        raise InvalidBlankLog('Only printable ASCII and LF log bytes are supported')
    lines = body.split(b'\n')
    partial = lines.pop()
    if any(len(v) > MAX_LINE_BYTES for v in lines) or len(partial) > MAX_LINE_BYTES:
        raise InvalidBlankLog('Blank startup line exceeds bound')
    history = None
    if len(lines) > 2 + MAX_CLEAN_FRAMES:
        raise InvalidBlankLog('Blank frame count exceeds capture protocol bound')
    for index, line in enumerate(lines):
        if index == 0:
            valid = line == recent
        elif index == 1:
            valid = line == native_ready
            if valid:
                history = expected_history_bytes
        else:
            valid = line == BLANK_FRAME
        if not valid:
            raise InvalidBlankLog('Unexpected or out-of-order blank startup record')
    if partial:
        index = len(lines)
        if index == 0:
            possible = recent.startswith(partial)
        elif index == 1:
            possible = native_ready.startswith(partial)
        elif index < 2 + MAX_CLEAN_FRAMES:
            possible = BLANK_FRAME.startswith(partial)
        else:
            possible = False
        if not possible:
            raise InvalidBlankLog('Unexpected trailing partial record')
    return BlankLogSnapshot(
        'CLEAN_BLANK_PREFIX' if len(lines) >= 3 and not partial else 'WAITING',
        len(body), hashlib.sha256(body).hexdigest(), history, max(0, len(lines)-2))


def require_unchanged_blank_snapshots(before, after, run_label, *, expected_history_bytes):
    """Compare two actual bounded reads; this still grants no runtime authority."""
    first = inspect_blank_log(before, run_label, expected_history_bytes=expected_history_bytes)
    second = inspect_blank_log(after, run_label, expected_history_bytes=expected_history_bytes)
    if first.state != 'CLEAN_BLANK_PREFIX' or second.state != 'CLEAN_BLANK_PREFIX':
        raise InvalidBlankLog('Both snapshots must contain the complete clean blank prefix')
    if before != after:
        raise InvalidBlankLog('Blank startup log changed across capture')
    return second
