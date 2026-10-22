"""Strict passive editor-observer V2 data; no guest execution or input authority."""
from dataclasses import dataclass, field
import re

SCREEN_TITLE = 'ProTracker 2.4G Enhanced Editor'
SCREEN_FIELDS = ('screen_width', 'screen_height', 'mouse_x', 'mouse_y', 'left', 'top',
                 'viewport_dx', 'viewport_dy', 'viewport_modes', 'screen_flags', 'default_title_state')
WINDOW_FIELDS = ('left', 'top', 'width', 'height', 'mouse_x', 'mouse_y', 'border_left',
                 'border_top', 'border_right', 'border_bottom', 'window_flags', 'title_state')
INTEGER = r'(0|-?[1-9][0-9]{0,9})'


@dataclass(frozen=True)
class EditorPointerSnapshot:
    screen: dict
    window: dict
    os_hotspot_mapping_accepted: bool = field(default=False, init=False)
    input_authorized: bool = field(default=False, init=False)


def _line(line, prefix, fields, title_field='title'):
    pattern = prefix + ' ' + ' '.join(k + '=' + INTEGER for k in fields)
    match = re.fullmatch(pattern + ' ' + title_field + r'_hex=(-|[0-9a-f]{2,62})', line)
    if not match:
        raise ValueError('Exact canonical V2 fields required')
    value = dict(zip(fields, map(int, match.groups()[:-1])))
    title = match.groups()[-1]
    state = value[title_field + '_state']
    if state in (0, 1) and title == '-':
        value['title'] = None if state == 0 else ''
    elif state == 2 and title != '-' and len(title) % 2 == 0:
        value['title'] = bytes.fromhex(title).decode('latin-1')
    else:
        raise ValueError('Title state and bounded hex must agree')
    return value


def parse_editor_pointer_snapshot(body):
    if (type(body) is not bytes or not 1 <= len(body) <= 2048
            or any(c != 10 and not 32 <= c <= 126 for c in body)):
        raise ValueError('Bounded ASCII observer reply required')
    lines = body.decode('ascii').splitlines()
    if lines and lines[0] == '[OK]':
        lines = lines[1:]
    if len(lines) != 2:
        raise ValueError('Exactly one screen and window record required')
    screen = _line(lines[0], 'PT_EDITOR_SCREEN_V2', SCREEN_FIELDS, 'default_title')
    window = _line(lines[1], 'PT_EDITOR_WINDOW_V2', WINDOW_FIELDS)
    if (screen['screen_width'] != 640 or screen['screen_height'] != 512
            or screen['left'] != 0 or screen['top'] != 0
            or screen['title'] != SCREEN_TITLE
            or not 0 <= screen['viewport_modes'] <= 65535
            or screen['viewport_modes'] & 0x8024 != 0x8004
            or not 0 <= screen['screen_flags'] <= 65535
            or screen['screen_flags'] & 15 != 15
            or not all(-32768 <= screen[k] <= 32767 for k in ('viewport_dx', 'viewport_dy'))
            or not 0 <= screen['mouse_x'] < 640 or not 0 <= screen['mouse_y'] < 512):
        raise ValueError('Actual source-bound 640x512 custom HIRES LACE editor screen required')
    if (window['left'] != 0 or window['top'] != 0 or window['width'] != 640
            or window['height'] != 512 or window['title_state'] not in (0, 1)
            or any(window[k] != 0 for k in ('border_left', 'border_top', 'border_right', 'border_bottom'))
            or not 0 <= window['window_flags'] <= 0xffffffff
            or window['window_flags'] & 0x900 != 0x900
            or window['mouse_x'] != screen['mouse_x'] or window['mouse_y'] != screen['mouse_y']):
        raise ValueError('Coherent untitled borderless backdrop editor window required')
    return EditorPointerSnapshot(screen, window)
