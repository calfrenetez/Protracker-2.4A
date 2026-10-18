"""Private HOST qualification of detached document preparation/cancellation.

Real document/project/editor/sampler/history implementation; no native SDK,
quiet/stop receipts, publication, GUI or target authority. Retained negative
cases stay rooted in fresh fixture processes. ASan/UBSan; LSan not available.
"""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import os
import posixpath
import re
import stat
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = None
SOURCES = (
    'tests/native_document_switch_test.c', 'src/native/editor_document_switch.c',
    'src/editor/editor.c', 'src/editor/song.c', 'src/editor/sampler.c',
    'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c',
    'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/core/document.c',
    'src/core/pp20.c', 'src/core/pattern.c', 'src/core/project.c',
    'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c',
    'src/core/pcm.c', 'src/editor/sampler_lifetime.c', 'src/editor/workflow.c',
    'src/editor/sample_range.c', 'src/editor/wave_summary.c',
    'src/core/sample_usage.c', 'src/core/event_resource.c', 'src/core/flow.c',
    'src/core/pitch.c', 'src/core/render.c', 'src/core/timeline.c',
    'src/core/frame_clock.c', 'src/core/voice.c',
)
CFLAGS = ('-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
          '-fsanitize=address,undefined', '-Isrc/core', '-I.')
CASES = (
    ('new_cancel_reuse', False), ('load_roundtrip', False),
    ('ordinary_failure', False), ('geometry', False), ('reentry', False),
    ('original_drift', True), ('callback_drift', True),
    ('candidate_drift', True), ('release_quarantine', True),
)
STAT_KEYS = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_uid', 'st_gid',
             'st_size', 'st_mtime_ns', 'st_ctime_ns', 'st_flags')

def source_bytes(relative):
    """Only bounded ordinary resident repository source, without symlink reads."""
    relative = PurePosixPath(relative)
    if relative.is_absolute() or '..' in relative.parts:
        raise AssertionError('source path escapes repository')
    path = ROOT / relative
    before = path.lstat()
    facts = lambda value: tuple(getattr(value, name, 0) for name in STAT_KEYS)
    if (not stat.S_ISREG(before.st_mode) or before.st_nlink != 1 or
            getattr(before, 'st_flags', 0) & 0x40000000 or before.st_size > 1024 * 1024):
        raise AssertionError('source must be ordinary, resident and bounded: ' + str(relative))
    descriptor = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        if facts(os.fstat(descriptor)) != facts(before):
            raise AssertionError('opened source identity changed')
        body = bytearray()
        while len(body) <= before.st_size:
            block = os.read(descriptor, min(65536, before.st_size + 1 - len(body)))
            if not block:
                break
            body.extend(block)
        if len(body) != before.st_size or facts(os.fstat(descriptor)) != facts(before):
            raise AssertionError('source changed while read')
    finally:
        os.close(descriptor)
    if facts(path.lstat()) != facts(before):
        raise AssertionError('source changed after read')
    return bytes(body), (hashlib.sha256(body).hexdigest(), facts(before))


class NativeDocumentPreparationHost(unittest.TestCase):
    def test_real_factories_and_cancellation(self):
        self.assertEqual(len(SOURCES), 31)
        self.assertEqual(len(CASES), 9)
        observed, children = {}, []
        receipt = {'state': 'NOT_RUN', 'children': children,
                   'sanitizers': 'ASAN_UBSAN', 'leakSanitizer': 'NOT_AVAILABLE_NOT_REQUESTED'}
        if EVIDENCE is not None:
            EVIDENCE.mkdir()

        def read(relative):
            body, facts = source_bytes(relative)
            if relative in observed:
                self.assertEqual(observed[relative], facts, relative)
            observed[relative] = facts
            return body

        def child(label, argv, private, environment, limit):
            result = subprocess.run(argv, cwd=private, env=environment,
                                    capture_output=True, timeout=limit)
            row = {'label': label, 'argv': list(argv), 'cwd': str(private),
                   'returncode': result.returncode,
                   'stdoutBytes': len(result.stdout), 'stdoutSHA256': hashlib.sha256(result.stdout).hexdigest(),
                   'stderrBytes': len(result.stderr), 'stderrSHA256': hashlib.sha256(result.stderr).hexdigest()}
            children.append(row)
            if EVIDENCE is not None:
                (EVIDENCE / (label + '.stdout')).write_bytes(result.stdout)
                (EVIDENCE / (label + '.stderr')).write_bytes(result.stderr)
                (EVIDENCE / 'children.json').write_text(json.dumps(children, indent=2) + '\n')
            return result

        try:
            with tempfile.TemporaryDirectory(prefix='pt-document-preparation-') as temporary:
                root = Path(temporary)
                private, work = root / 'source', root / 'tmp'
                private.mkdir(); work.mkdir()
                copied = set()

                def copy(relative):
                    if relative in copied:
                        return
                    self.assertLess(len(copied), 128, 'finite source closure exceeded')
                    data = read(relative); copied.add(relative)
                    destination = private / relative
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    destination.write_bytes(data)
                    for name in re.findall(rb'^\s*#\s*include\s*"([^"\r\n]+)"', data, re.MULTILINE):
                        name = name.decode()
                        choices = (posixpath.normpath(str(PurePosixPath(relative).parent / name)),
                                   posixpath.normpath('src/core/' + name), posixpath.normpath(name))
                        for candidate in dict.fromkeys(choices):
                            if candidate.startswith('../') or candidate.startswith('/'):
                                continue
                            if (ROOT / candidate).is_file():
                                copy(candidate)
                                break
                        else:
                            self.fail('unresolved quoted include: ' + relative + ': ' + name)

                for relative in SOURCES:
                    copy(relative)
                environment = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(work),
                               'ASAN_OPTIONS': 'detect_leaks=0:halt_on_error=1:abort_on_error=1',
                               'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
                executable = root / 'fixture'
                compiled = child('compile', ['/usr/bin/cc', *CFLAGS, *SOURCES, '-o', str(executable)],
                                 private, environment, 90)
                self.assertEqual(compiled.returncode, 0, compiled.stderr.decode(errors='replace'))
                self.assertEqual(compiled.stdout, b''); self.assertEqual(compiled.stderr, b'')
                for name, retained in CASES:
                    actual = child(name, [str(executable), name], private, environment, 5)
                    expected = ('DOCUMENT PREP HOST ' + name + ' PASS ' +
                                ('RETAINED_DOMAIN' if retained else 'COMPLETE_DOMAIN') + '\n').encode()
                    self.assertEqual(actual.returncode, 0, actual.stderr.decode(errors='replace'))
                    self.assertEqual(actual.stdout, expected); self.assertEqual(actual.stderr, b'')
                for relative, facts in observed.items():
                    self.assertEqual(source_bytes(relative)[1], facts, relative)
                receipt.update(state='PRIVATE_HOST_DOCUMENT_PREPARATION_PASS_NOT_NATIVE_NOT_PUBLICATION',
                               sourceCount=len(observed), sources=observed,
                               allSourceFactsEqual=True, completeCases=5, retainedCases=4)
        except BaseException as error:
            receipt.update(state='PRIVATE_HOST_DOCUMENT_PREPARATION_FAILED', error=repr(error),
                           sources=observed)
            raise
        finally:
            if EVIDENCE is not None:
                (EVIDENCE / 'result.json').write_text(json.dumps(receipt, indent=2) + '\n')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--evidence', type=Path)
    options, remaining = parser.parse_known_args()
    EVIDENCE = options.evidence
    unittest.main(argv=[sys.argv[0], *remaining])
