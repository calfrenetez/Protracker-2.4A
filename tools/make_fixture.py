#!/usr/bin/env python3
"""Generate original, redistributable four-channel MOD smoke material."""
from pathlib import Path
import argparse
import hashlib
import json
import struct


def make_mod():
    data = bytearray(1084 + 1024)
    data[:20] = b'PT BASELINE SMOKE'.ljust(20, b'\0')
    data[20:42] = b'SYNTHETIC TRIANGLE'.ljust(22, b'\0')
    struct.pack_into('>HBBHH', data, 42, 32, 0, 24, 0, 32)
    for i in range(1, 31):
        struct.pack_into('>H', data, 20 + i * 30 + 28, 1)
    data[950] = 1
    data[951] = 127
    data[1080:1084] = b'M.K.'
    for row in range(0, 64, 8):
        for ch, period in enumerate((428, 339, 285, 214)):
            off = 1084 + row * 16 + ch * 4
            data[off:off+4] = bytes((period >> 8, period & 255, 0x10, 0))
    # Final row returns to song position zero; no external material used.
    data[1084 + 63 * 16 + 2] = 0x0B
    triangle = [i * 4 - 64 for i in range(32)]
    data.extend(value & 255 for value in triangle + triangle[::-1])
    return bytes(data)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output', type=Path)
    args = p.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    data = make_mod()
    args.output.write_bytes(data)
    print(json.dumps({'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}))
