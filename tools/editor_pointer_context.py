"""Strict V3 guest coordinate context; data only, no execution or input authority.

The Window pointer fields describe an override only when Pointer is non-NULL.
They cannot identify a system-default pointer's hotspot. Active flags and
IntuitionBase coordinates are observations, not focus or mapping acceptance.
"""
from dataclasses import dataclass, field
import re

from editor_pointer_snapshot import parse_editor_pointer_snapshot, INTEGER

CONTEXT_FIELDS = (
    'view_dx', 'view_dy', 'intuition_mouse_x', 'intuition_mouse_y',
    'active_screen', 'active_window', 'window_pointer_override',
    'window_pointer_width', 'window_pointer_height',
    'window_pointer_x_offset', 'window_pointer_y_offset',
)
WORDS = CONTEXT_FIELDS[:4]
FLAGS = CONTEXT_FIELDS[4:7]
BYTES = CONTEXT_FIELDS[7:]


@dataclass(frozen=True)
class EditorPointerContext:
    screen: dict
    window: dict
    context: dict
    os_hotspot_mapping_accepted: bool = field(default=False, init=False)
    input_authorized: bool = field(default=False, init=False)


def parse_editor_pointer_context(body):
    if (type(body) is not bytes or not 1 <= len(body) <= 3072
            or any(c != 10 and not 32 <= c <= 126 for c in body)):
        raise ValueError('Bounded ASCII V3 diagnostic reply required')
    lines = body.decode('ascii').splitlines()
    if lines and lines[0] == '[OK]':
        lines = lines[1:]
    if (len(lines) != 3 or not lines[0].startswith('PT_EDITOR_SCREEN_V3 ')
            or not lines[1].startswith('PT_EDITOR_WINDOW_V3 ')):
        raise ValueError('Exactly one V3 screen, window and context required')
    # Reuse every existing source-bound screen/window check without changing V2.
    pair = (lines[0].replace('PT_EDITOR_SCREEN_V3 ', 'PT_EDITOR_SCREEN_V2 ', 1)
            + '\n' + lines[1].replace('PT_EDITOR_WINDOW_V3 ', 'PT_EDITOR_WINDOW_V2 ', 1)
            + '\n').encode('ascii')
    original = parse_editor_pointer_snapshot(pair)
    pattern = 'PT_EDITOR_CONTEXT_V3 ' + ' '.join(k + '=' + INTEGER for k in CONTEXT_FIELDS)
    match = re.fullmatch(pattern, lines[2])
    if not match:
        raise ValueError('Exact canonical V3 context fields required')
    context = dict(zip(CONTEXT_FIELDS, map(int, match.groups())))
    if (any(not -32768 <= context[k] <= 32767 for k in WORDS)
            or any(context[k] not in (0, 1) for k in FLAGS)
            or any(not -128 <= context[k] <= 127 for k in BYTES)):
        raise ValueError('Native WORD, BYTE and boolean context ranges required')
    return EditorPointerContext(original.screen, original.window, context)
