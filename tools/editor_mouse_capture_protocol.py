"""Finite first-mouse observation protocol; no runtime or input authority.

The caller must separately bind the actual candidate, capture registry, owner,
native window and source geometry. These functions neither send input nor
accept playback, exit, restoration or whole-application behavior.
"""
from dataclasses import dataclass
import hashlib

from editor_blank_capture_protocol import (
    BLANK_FRAME, MAX_CLEAN_FRAMES, MAX_LINE_BYTES, MAX_LOG_BYTES,
    InvalidBlankLog, inspect_blank_log,
)

SELECTED_FRAME = (b'EDITOR FRAME row=4 channel=1 revision=0 dirty=0 status=READY - '
                  b'F8 PLAY / F9 PATTERN / F10 STOP panel=0')


@dataclass(frozen=True)
class SelectionSnapshot:
    state: str
    sha256: str
    selected_frames: int


def inspect_first_selection(body, baseline, run_label, *, expected_history_bytes):
    """Require the exact clean baseline followed only by the planned selection.

    Refreshes may repeat the initial frame before selection. Once selection
    appears, a return to the original frame is refused. Every incomplete record
    remains WAITING, including one following a previously complete selection.
    """
    initial = inspect_blank_log(
        baseline, run_label, expected_history_bytes=expected_history_bytes)
    if initial.state != 'CLEAN_BLANK_PREFIX':
        raise InvalidBlankLog('Complete actual blank baseline required')
    if (not isinstance(body, bytes) or len(body) > MAX_LOG_BYTES
            or not body.startswith(baseline)):
        raise InvalidBlankLog('Selection must extend the exact bounded baseline')
    tail = body[len(baseline):]
    if any(v != 10 and not 32 <= v <= 126 for v in tail):
        raise InvalidBlankLog('Selection log must be printable ASCII and LF')
    records = tail.split(b'\n')
    partial = records.pop()
    if any(len(v) > MAX_LINE_BYTES for v in records + [partial]):
        raise InvalidBlankLog('Selection line exceeds bound')
    if initial.clean_frames + len(records) > MAX_CLEAN_FRAMES:
        raise InvalidBlankLog('Selection frame count exceeds bound')
    selected = 0
    for line in records:
        if line == SELECTED_FRAME:
            selected += 1
        elif line != BLANK_FRAME or selected:
            raise InvalidBlankLog('Unexpected selection, action or frame reversion')
    if partial:
        allowed = (SELECTED_FRAME,) if selected else (BLANK_FRAME, SELECTED_FRAME)
        if (initial.clean_frames + len(records) >= MAX_CLEAN_FRAMES
                or not any(line.startswith(partial) for line in allowed)):
            raise InvalidBlankLog('Unexpected trailing partial selection record')
    return SelectionSnapshot(
        'SELECTED_ROW4_CHANNEL1' if selected and not partial else 'WAITING',
        hashlib.sha256(body).hexdigest(), selected)


def first_selection_point(screen_rectangle, image_size, *, native_size):
    """Map the approved cell center through an observed unscaled editor rect.

    The rectangle must come from this exact Root-viewed capture, and native_size
    must be independently bound to its actual editor screen. Neither is fitted
    to pointer coordinates. No Workbench scale, offset compensation or
    cursor tolerance is admitted. Runtime pointer equality is a separate gate.
    """
    integer = lambda v: type(v) is int
    if (type(screen_rectangle) not in (tuple, list) or len(screen_rectangle) != 4
            or not all(integer(v) for v in screen_rectangle)
            or type(image_size) not in (tuple, list) or len(image_size) != 2
            or not all(integer(v) and v > 0 for v in image_size)
            or type(native_size) not in (tuple, list) or len(native_size) != 2
            or not all(integer(v) for v in native_size)
            or tuple(native_size) != (640, 512)):
        raise ValueError('Observed integer editor rectangle and image size required')
    x, y, width, height = screen_rectangle
    if (width != 640 or height != 512 or x < 0 or y < 0
            or x + width > image_size[0] or y + height > image_size[1]):
        raise ValueError('Exact unscaled640x512 editor rectangle required')
    # Native source: pattern y250,20rows,12px/row,initial first_row=-10;
    # channels start x38,150px each, note field first60px. Center218,424
    # selects channel1/row4/field0 without changing revision or dirty state.
    return (x + 218, y + 424)
