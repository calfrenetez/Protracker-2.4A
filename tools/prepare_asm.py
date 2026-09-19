#!/usr/bin/env python3
"""Spell immediate operations explicitly for the locked AsmOne-to-vasm build.

Only a generated build copy changes. Never use vasm's generic encodings as
evidence of equivalence: the official code hunks are the acceptance reference.
"""
import collections
import re

IMMEDIATE = re.compile(
    r'^((?:[A-Za-z_.][A-Za-z0-9_.]*:?)?\s+)'
    r'(CMP|AND|OR|ADD|SUB)(\.[BWL]\s+#[^;\n]+,\s*D[0-7])(?=\s*(?:;|$))',
    re.MULTILINE | re.IGNORECASE,
)


def prepare(data):
    counts = collections.Counter()

    def replace(match):
        counts[match[2].upper()] += 1
        return match[1] + match[2] + 'I' + match[3]

    return IMMEDIATE.sub(replace, data.decode('latin1')).encode('latin1'), dict(counts)
