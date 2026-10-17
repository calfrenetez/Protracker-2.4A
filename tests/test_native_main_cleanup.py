"""HOST regression of actual native-main cleanup decisions and genuine owners.

No native SDK, Wait/HOLD simulation, GUI/device or AppSegment authority. Six
negative cases retain original owners until their fresh fixture process ends.
ASan/UBSan are enabled; leak-sanitizer is deliberately not requested.
"""
from pathlib import Path, PurePosixPath
import hashlib
import os
import posixpath
import re
import stat
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = (
    'tests/native_main_cleanup_test.c',
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
    ('startup_empty', False), ('success_history', False), ('editor_veto', True),
    ('sampler_pin', True), ('progress_retained', True), ('workflow_cancel', False),
    ('recovery_admission', False), ('recovery_residue', False),
    ('ledger_mismatch', True), ('source_quarantine', True), ('root_quarantine', True),
)
SELECTIONS = (
    ('src/native/editor_main.c', 'tests/main-cleanup.selected.h', (
        'static int editor_init_memory(', 'static int cleanup_editor_owners(',
        'static int cleanup_recovery_result(', 'static int cleanup_return_code(')),
    ('src/native/recovery.c', 'tests/recovery-finish.selected.h', (
        'int pt_native_recovery_finish(',)),
    ('src/platform/recovery_store.c', 'tests/recovery-store.selected.h', (
        'static int new_directory(', 'static int empty_directory(',
        'static int remove_owned(', 'enum pt_save_result pt_recovery_store_create(',
        'int pt_recovery_store_discard(')),
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


def definition(body, signature):
    """Select one actual definition in these sources' column-zero C style.

    The current single-line declaration and next-line brace must match exactly;
    prototypes and intervening prose refuse. Nested braces are indented. This
    is selection of these known definitions, not a general C parser.
    """
    pattern = re.compile(rb'^' + re.escape(signature.encode()) +
                         rb'[^\r\n;{}]*\)[ \t]*\n\{\n', re.MULTILINE)
    starts = list(pattern.finditer(body))
    if len(starts) != 1:
        raise AssertionError('one actual definition required: ' + signature)
    end = re.search(rb'^}\r?$', body[starts[0].end():], re.MULTILINE)
    if end is None:
        raise AssertionError('definition closing brace missing: ' + signature)
    stop = starts[0].end() + end.end()
    return body[starts[0].start():stop] + b'\n'


class NativeMainCleanupHost(unittest.TestCase):
    def test_actual_cleanup_decisions(self):
        self.assertEqual(len(SOURCES), 30)
        self.assertEqual(len(CASES), 11)
        observed, selected = {}, {}

        def read(relative):
            body, facts = source_bytes(relative)
            if relative in observed:
                self.assertEqual(observed[relative], facts, relative)
            observed[relative] = facts
            return body

        for origin, destination, signatures in SELECTIONS:
            body = read(origin)
            selected[destination] = b'\n'.join(definition(body, signature) for signature in signatures)

        # Bind the real native caller's order, while the C fixture executes the
        # actual selected decisions with genuine editor/document/workflow APIs.
        main = read('src/native/editor_main.c')
        self.assertEqual(main.count(b'\ndone:\n'), 1)
        tail = main.split(b'\ndone:\n', 1)[1]
        order = (
            b'pt_paula_stop(&audio);',
            b'recovery_clean=cleanup_recovery_result(&recovery,normal_exit);',
            b'if(recovery_clean<0)cleanup_owner_hold();',
            b'if(!cleanup_editor_owners(editor,&doc))cleanup_owner_hold();',
            b'if(clock_pending)', b'if(window)CloseWindow(window);',
            b'screen_close_or_hold(&screen);', b'if(pixels)FreeMem',
            b'if(GfxBase)CloseLibrary', b'if(IntuitionBase)CloseLibrary',
            b'pt_master_release(&master_memory,editor);',
            b'rc=cleanup_return_code(normal_exit,recovery_clean);',
            b'if(rc<0)cleanup_owner_hold();', b'if(rc==0)puts("EDITOR EXIT clean");',
            b'return rc;',
        )
        positions = [tail.index(item) for item in order]
        self.assertEqual(positions, sorted(positions))
        self.assertNotIn(b'rc=0;', tail)

        with tempfile.TemporaryDirectory(prefix='pt-main-cleanup-') as temporary:
            root = Path(temporary)
            private, work = root / 'source', root / 'tmp'
            private.mkdir(); work.mkdir()
            copied = set()

            def copy(relative):
                if relative in copied:
                    return
                self.assertLess(len(copied), 128, 'finite source closure exceeded')
                data = selected[relative] if relative in selected else read(relative)
                copied.add(relative)
                path = private / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
                for name in re.findall(rb'^\s*#\s*include\s*"([^"\r\n]+)"', data, re.MULTILINE):
                    name = name.decode()
                    choices = (posixpath.normpath(str(PurePosixPath(relative).parent / name)),
                               posixpath.normpath('src/core/' + name), posixpath.normpath(name))
                    for candidate in dict.fromkeys(choices):
                        if candidate.startswith('../') or candidate.startswith('/'):
                            continue
                        if candidate in selected or (ROOT / candidate).is_file():
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
            compiled = subprocess.run(['/usr/bin/cc', *CFLAGS, *SOURCES, '-o', str(executable)],
                                      cwd=private, env=environment, capture_output=True, timeout=90)
            self.assertEqual(compiled.returncode, 0, compiled.stderr.decode(errors='replace'))
            self.assertEqual(compiled.stdout, b'')
            self.assertEqual(compiled.stderr, b'')
            for name, retained in CASES:
                with self.subTest(case=name):
                    actual = subprocess.run([str(executable), name, str(work)], cwd=private,
                                            env=environment, capture_output=True, timeout=5)
                    self.assertEqual(actual.returncode, 0, actual.stderr.decode(errors='replace'))
                    expected = ('MAIN CLEANUP HOST ' + name + ' PASS ' +
                                ('RETAINED_DECISION' if retained else 'COMPLETE_DECISION') + '\n')
                    self.assertEqual(actual.stdout, expected.encode())
                    self.assertEqual(actual.stderr, b'')
            for relative, facts in observed.items():
                self.assertEqual(source_bytes(relative)[1], facts, relative)


if __name__ == '__main__':
    unittest.main()
