"""Bounded saved-image cursor colour inspection, never input or hotspot proof.

The CLI needs Pillow in its interpreter. The packed-RGB core is dependency-free.
Always view the original current image separately. A painted sprite boundary
does not establish the OS pointer hotspot, authorize a click or correct input.
"""
from dataclasses import asdict, dataclass, field
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import stat

MAX_PIXELS = 2_000_000
MAX_REGION_PIXELS = 4096
MAX_PAINTED_PIXELS = 1024
MAX_PNG_BYTES = 8 * 1024 * 1024
# Observed colours in the retained default pointer, not a universal OS palette.
CURSOR_COLOURS = ((224, 64, 64), (224, 224, 192))


@dataclass(frozen=True)
class PaintedBoundary:
    state: str
    first_painted_pixel: tuple | None
    painted_bounds: tuple | None
    painted_pixels: int
    inspected_rectangle: tuple
    ascii_rows: tuple
    os_hotspot_established: bool = field(default=False, init=False)
    click_authorized: bool = field(default=False, init=False)


def inspect_packed_rgb(before, after, size, rectangle, *, colours=CURSOR_COLOURS):
    """Find one connected new painted component inside a disclosed rectangle.

    Coordinates are original image pixels. No inferred offset, nearest target,
    scaling tolerance, expected hotspot or application acceptance is used.
    """
    integer = lambda n: type(n) is int
    if (type(size) not in (tuple, list) or len(size) != 2
            or not all(integer(n) and n > 0 for n in size)
            or size[0] * size[1] > MAX_PIXELS
            or type(before) is not bytes or type(after) is not bytes
            or len(before) != size[0] * size[1] * 3 or len(after) != len(before)):
        raise ValueError('Equal bounded packed RGB images required')
    if (type(rectangle) not in (tuple, list) or len(rectangle) != 4
            or not all(integer(n) for n in rectangle)):
        raise ValueError('Integer inspected rectangle required')
    x0, y0, width, height = rectangle
    if (min(x0, y0) < 0 or min(width, height) <= 0
            or width * height > MAX_REGION_PIXELS
            or x0 + width > size[0] or y0 + height > size[1]):
        raise ValueError('Inspected rectangle must be bounded inside the image')
    if (type(colours) not in (tuple, list) or not 1 <= len(colours) <= 8
            or any(type(c) not in (tuple, list) or len(c) != 3
                   or not all(integer(n) and 0 <= n <= 255 for n in c)
                   for c in colours)):
        raise ValueError('Explicit bounded RGB palette required')
    palette = tuple(tuple(c) for c in colours)
    if len(set(palette)) != len(palette):
        raise ValueError('Palette entries must be distinct')
    painted = {}
    for y in range(y0, y0 + height):
        for x in range(x0, x0 + width):
            at = (y * size[0] + x) * 3
            rgb = tuple(after[at:at + 3])
            if rgb in palette and before[at:at + 3] != after[at:at + 3]:
                painted[x, y] = palette.index(rgb)
    if len(painted) > MAX_PAINTED_PIXELS:
        raise ValueError('Painted component exceeds inspection bound')
    if not painted:
        return PaintedBoundary('NO_NEW_PALETTE_COMPONENT', None, None, 0,
                               tuple(rectangle), ())
    remaining = set(painted)
    pending = [next(iter(remaining))]
    remaining.remove(pending[0])
    while pending:
        x, y = pending.pop()
        for other in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
            if other in remaining:
                remaining.remove(other)
                pending.append(other)
    if remaining:
        raise ValueError('Multiple painted components; no unique boundary')
    left = min(x for x, _ in painted); right = max(x for x, _ in painted)
    top = min(y for _, y in painted); bottom = max(y for _, y in painted)
    first = (min(x for x, y in painted if y == top), top)
    rows = tuple(''.join(str(painted[x, y]) if (x, y) in painted else '.'
                         for x in range(left, right + 1))
                 for y in range(top, bottom + 1))
    return PaintedBoundary('ONE_PAINTED_COMPONENT_ONLY_NOT_OS_HOTSPOT', first,
                           (left, top, right - left + 1, bottom - top + 1),
                           len(painted), tuple(rectangle), rows)


def _resident_png(path):
    path = Path(path).absolute()
    before = path.lstat()
    fields = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_size',
              'st_mtime_ns', 'st_ctime_ns', 'st_flags')
    facts = lambda s: {k: getattr(s, k, 0) for k in fields}
    if (not stat.S_ISREG(before.st_mode) or before.st_nlink != 1
            or before.st_size > MAX_PNG_BYTES
            or getattr(before, 'st_flags', 0) & 0x40000000):
        raise ValueError('Bounded resident ordinary PNG required')
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        if facts(os.fstat(fd)) != facts(before):
            raise ValueError('Image endpoint changed before read')
        with os.fdopen(fd, 'rb', closefd=False) as stream:
            body = stream.read(MAX_PNG_BYTES + 1)
        if (len(body) != before.st_size or facts(os.fstat(fd)) != facts(before)
                or facts(path.lstat()) != facts(before)):
            raise ValueError('Image endpoint changed during read')
    finally:
        os.close(fd)
    from PIL import Image
    with Image.open(io.BytesIO(body)) as image:
        if (image.format != 'PNG' or image.mode not in ('RGB', 'RGBA')
                or image.width * image.height > MAX_PIXELS):
            raise ValueError('Bounded RGB or RGBA PNG required')
        if image.mode == 'RGBA' and image.getextrema()[3] != (255, 255):
            raise ValueError('Transparent pixels require separate interpretation')
        size = image.size
        rgb = image.convert('RGB').tobytes()
    return rgb, size, {'path': str(path), 'bytes': len(body),
                       'sha256': hashlib.sha256(body).hexdigest(),
                       'factsAtRead': facts(before)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', required=True)
    parser.add_argument('--after', required=True)
    parser.add_argument('--rectangle', nargs=4, type=int, required=True,
                        metavar=('X', 'Y', 'WIDTH', 'HEIGHT'))
    args = parser.parse_args()
    before, size, before_ref = _resident_png(args.before)
    after, after_size, after_ref = _resident_png(args.after)
    if after_size != size:
        raise ValueError('Actual image dimensions must agree')
    result = inspect_packed_rgb(before, after, size, args.rectangle)
    print(json.dumps({'inspection': asdict(result), 'size': size,
                      'before': before_ref, 'after': after_ref,
                      'palette': CURSOR_COLOURS, 'targetCalls': 0,
                      'rootObservationPublished': False}, sort_keys=True))


if __name__ == '__main__':
    main()
