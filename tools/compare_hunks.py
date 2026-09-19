#!/usr/bin/env python3
"""Compare the baseline's exact loaded bytes, hunk layout and relocations.

This is a static assembler comparison, not a runtime compatibility certificate.
Unknown hunk records fail closed. Intended for the locked stripped executables.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def parse(data):
    cursor = 0

    def word():
        nonlocal cursor
        if cursor + 4 > len(data):
            raise ValueError('Truncated hunk')
        value = struct.unpack_from('>I', data, cursor)[0]
        cursor += 4
        return value

    if word() != 1011 or word() != 0:
        raise ValueError('Expected unnamed HUNK_HEADER')
    table, first, last = word(), word(), word()
    if first != 0 or last + 1 != table or table > len(data) // 4:
        raise ValueError('Invalid hunk table')
    sizes = [word() for _ in range(table)]
    hunks = []
    for allocation in sizes:
        kind, length = word(), word()
        if kind not in (1001, 1002, 1003):
            raise ValueError('Unsupported section kind')
        length *= 4
        if length > (allocation & 0x3fffffff) * 4:
            raise ValueError('Section exceeds allocation')
        body = b''
        if kind != 1003:
            if cursor + length > len(data):
                raise ValueError('Truncated section')
            body = data[cursor:cursor+length]
            cursor += length
        relocs = []
        while True:
            record = word()
            if record == 1010:
                break
            if record != 1004:
                raise ValueError('Unsupported record: ' + str(record))
            while True:
                count = word()
                if count == 0:
                    break
                target = word()
                if target >= table or count > len(data) // 4:
                    raise ValueError('Invalid relocation group')
                for _ in range(count):
                    offset = word()
                    if offset + 4 > length or offset % 2:
                        raise ValueError('Invalid relocation offset')
                    relocs.append((target, offset))
        if len(set(relocs)) != len(relocs):
            raise ValueError('Duplicate relocation')
        hunks.append((kind, allocation, length, body, sorted(relocs)))
    if cursor != len(data):
        raise ValueError('Trailing bytes')
    return hunks


def compare(local, official):
    a, b = parse(local), parse(official)
    if len(a) != len(b):
        raise ValueError('Different hunk counts')
    sections = []
    for index, (x, y) in enumerate(zip(a, b)):
        if x[:3] != y[:3] or x[4] != y[4]:
            raise ValueError('Hunk layout/relocations differ: ' + str(index))
        if x[3] != y[3]:
            raise ValueError('Loaded section bytes differ: ' + str(index))
        sections.append({'index': index, 'kind': x[0], 'bytes': x[2],
                         'relocations': len(x[4]), 'allocation': hex(x[1])})
    return {'identical_files': local == official, 'layout_and_relocations_match': True,
            'all_loaded_code_and_data_bytes_match': True,
            'sections': sections,
            'local_sha256': hashlib.sha256(local).hexdigest(),
            'official_sha256': hashlib.sha256(official).hexdigest()}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('local', type=Path)
    parser.add_argument('official', type=Path)
    args = parser.parse_args()
    print(json.dumps(compare(args.local.read_bytes(), args.official.read_bytes()), indent=2))
