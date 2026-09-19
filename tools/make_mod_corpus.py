#!/usr/bin/env python3
"""Generate original MOD fixtures, including explicitly unsafe malformed cases.

Run malformed cases through PTModCheck only until the native loader is hardened.
"""
import argparse
import hashlib
import json
from pathlib import Path
from make_fixture import make_mod


def cases():
    normal = make_mod()
    def patterns(count, marker):
        header = bytearray(normal[:1084])
        header[952] = count - 1
        header[1080:1084] = marker
        return bytes(header) + normal[1084:2108] * count + normal[2108:]

    yield 'valid-1', normal, 0, 'ok'
    yield 'valid-64', patterns(64, b'M.K.'), 0, 'ok'
    yield 'valid-100', patterns(100, b'M!K!'), 0, 'ok'
    header = bytearray(normal[:1084])
    header[1079] = 1
    yield 'valid-inactive-order', bytes(header) + normal[1084:2108] * 2 + normal[2108:], 0, 'ok'
    yield 'invalid-short-header', normal[:1083], 20, 'short-header'
    yield 'invalid-short-pattern', normal[:2107], 20, 'short-patterns'
    yield 'invalid-short-sample', normal[:-1], 20, 'short-samples'
    yield 'invalid-256-patterns', patterns(256, b'M!K!'), 20, 'pattern-limit'
    for length in [0, 129]:
        data = bytearray(normal); data[950] = length
        yield f'invalid-order-count-{length}', bytes(data), 20, 'bad-song-length'
    data = bytearray(normal); data[1084] = 0x20; data[1086] &= 0x0f
    yield 'invalid-instrument-32', bytes(data), 20, 'bad-instrument'
    data = bytearray(normal); data[1080:1084] = b'8CHN'
    yield 'unsupported-8-channel', bytes(data), 20, 'unsupported-format'
    yield 'warning-trailing-data', normal + b'preserve-this-extension', 5, 'ok'
    data = bytearray(normal); data[45] = 65
    yield 'warning-volume', bytes(data), 5, 'ok'
    data = bytearray(normal); data[46:50] = b'\xff\xff\xff\xff'
    yield 'warning-loop', bytes(data), 5, 'ok'


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    manifest = []
    for name, data, rc, status in cases():
        path = out / ('mod.' + name)
        path.write_bytes(data)
        manifest.append({'file': path.name, 'bytes': len(data),
                         'sha256': hashlib.sha256(data).hexdigest(),
                         'expected_return_code': rc, 'expected_status': status})
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output', type=Path)
    generate(p.parse_args().output)
