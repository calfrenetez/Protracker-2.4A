"""Complete-channel-message recording HOST qualification; no CAMD/native/output.

Only the eight selected TUs and their exact sixteen-body project closure are copied.
Unchanged recorder/pattern/project cores supply genuine storage, undo and save.
"""
from pathlib import Path
import hashlib
import types
import os
import stat
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT
HELPER = CANONICAL / 'tests/test_native_mixed_causal_rolling_entry.py'
HELPER_SHA = 'f73a23edf8c4591dceb44ade3b08d2e1c4c75cd54301b58267d27753f16d58c1'
SOURCES = ('tests/record_channel_test.c', 'src/core/record_channel.c', 'src/core/record.c', 'src/core/record_pattern.c', 'src/core/pattern.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c')
CFLAGS = ('-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
          '-fsanitize=address,undefined', '-Isrc/core', '-Itests', '-I.')
FROZEN = {'src/core/channels.c': 'b6316f4a19e737d3f5a19bd04f22f43b0da72188d2a0623404dc4a27bc9c622d', 'src/core/channels.h': '9eadeee6befb0f291e62c654a8ccd5ed6c6e7030f8c8c93587566fb06eb99517', 'src/core/pattern.c': '0d22aff4b63fae85fa44f063ba6c2eeba37467843e4fcceb493037036366431e', 'src/core/pattern.h': 'f8c9abde09c4fe495ae77f25c070da647eab038845729edc83adec08c5678d53', 'src/core/pcm.c': 'ddb208ff5dd445c73d1b573eac1aed3e74bc654fefc2662627d4c0d760f99266', 'src/core/pcm.h': '9b6996caf7c81bd53034f53e008e7681885ede2711dddf7c2027c3bab06cb10e', 'src/core/pcm_internal.h': '1abbf89dfc2b640624d3f2cb16c2c715d818deb4a88f5284383d386a35c0188b', 'src/core/project.c': 'f1bad97dc044e3976936be95c4e42051e8b7a79aab643a1c984a14a1dd24c75a', 'src/core/project.h': 'e1a749800258b1f1db1dedcd7d5d36970abbf7494279cb3669212757f982e21a', 'src/core/record.c': '891dca6c8ec9f3159ccd0a6ed08f04cf2900b977d95211d257771b9346ae0890', 'src/core/record.h': '04576dd8ce3d6437c141d689d6aed88a5754738aac8972f5104abf4b0278369f', 'src/core/record_channel.c': '54a20cd3b55b663ba7b616b4a232f6d802a6a5eba152e9a9caef3377c3cb5e28', 'src/core/record_channel.h': 'fbe1b23f057721ea99774e3230bc2b36dbac376386fe8372f83966d46a4e7c3b', 'src/core/record_pattern.c': 'b7ff69d4a7c34baa7f33b06734064b5be82f3e3d7bb3792ed2453b0cc6521165', 'src/core/record_pattern.h': '305e23411b91d9c10de377d046adb9661ef94bf2ab4b6eae947eda87b3d34233', 'tests/record_channel_test.c': '4e6501cc20859be9929bdb05332e9c1db71272951b38809eb494dc2c5e2f3afd'}
CORE_INPUTS = ('src/core/channels.c', 'src/core/channels.h', 'src/core/pattern.c', 'src/core/pattern.h', 'src/core/pcm.c', 'src/core/pcm.h', 'src/core/pcm_internal.h', 'src/core/project.c', 'src/core/project.h', 'src/core/record.c', 'src/core/record.h', 'src/core/record_pattern.c', 'src/core/record_pattern.h')
ORACLE = 'RECORD CHANNEL HOST PASS:6 groups;complete messages;filter/Hold;row63/zeroOff;real project/save/undo;collision/stale;explicit tracks;NO CAMD/TIMING/OUTPUT\n'
ORACLE_BYTES = 155


def load_pinned_helper():
    before = HELPER.lstat()
    assert stat.S_ISREG(before.st_mode) and not before.st_flags & 0x40000000
    names = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_size', 'st_uid',
             'st_gid', 'st_mtime_ns', 'st_ctime_ns', 'st_flags')
    facts = lambda value: tuple(getattr(value, name, 0) for name in names)
    descriptor = os.open(HELPER, os.O_RDONLY | os.O_NOFOLLOW)
    try:
        assert facts(os.fstat(descriptor)) == facts(before)
        body = bytearray()
        while True:
            chunk = os.read(descriptor, 65536)
            if not chunk:
                break
            body.extend(chunk)
        assert facts(os.fstat(descriptor)) == facts(before)
    finally:
        os.close(descriptor)
    assert facts(HELPER.lstat()) == facts(before)
    assert len(body) == before.st_size and hashlib.sha256(body).hexdigest() == HELPER_SHA
    base = types.ModuleType('pt_record_channel_frozen_helper')
    base.__file__ = str(HELPER)
    exec(compile(bytes(body), str(HELPER), 'exec'), base.__dict__)
    assert base.ROOT == CANONICAL and base.CFLAGS == CFLAGS
    return base


class RecordChannelHost(unittest.TestCase):
    def test_complete_messages_reach_genuine_recording_journal(self):
        base = load_pinned_helper()
        before = base.preservation_snapshot()
        base.evidence('record-channel-before-preservation.json', before)
        helper_before, _ = base.ordinary_pin(HELPER)
        wrapper_before, _ = base.ordinary_pin(Path(__file__))
        old_units = base.SOURCES
        canonical_inputs = {}
        private_inputs = {}
        pins = {}
        try:
            self.assertEqual(len(SOURCES), 8)
            self.assertEqual(len(set(SOURCES)), 8)
            self.assertEqual(len(FROZEN), 16)
            self.assertEqual(len(ORACLE.encode('ascii')), ORACLE_BYTES)
            for relative in CORE_INPUTS:
                fact, _ = base.ordinary_pin(CANONICAL / relative)
                self.assertEqual(fact['sha256'], FROZEN[relative], 'genuine canonical recorder/project dependency changed')
                canonical_inputs[relative] = fact
            for relative, sha in FROZEN.items():
                fact, _ = base.ordinary_pin(ROOT / relative)
                self.assertEqual(fact['sha256'], sha)
                private_inputs[relative] = fact
            base.ROOT = ROOT
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-record-channel-host-') as temporary:
                private = Path(temporary)
                pins = base.source_snapshot(private)
                self.assertEqual(set(pins), set(FROZEN))
                for relative, sha in FROZEN.items():
                    self.assertEqual(pins[relative]['sha256'], sha)
                environment = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(private),
                    'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1',
                    'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
                executable = private / 'fixture'
                command = ['/usr/bin/cc', *CFLAGS, *SOURCES, '-o', str(executable)]
                base.evidence('record-channel-compile-command.json', {'argv': command,
                    'cwd': str(private), 'env': environment, 'timeoutSeconds': 120})
                start = time.monotonic()
                compiled = subprocess.run(command, cwd=private, env=environment,
                    capture_output=True, timeout=120)
                compile_seconds = time.monotonic()-start
                base.evidence('record-channel-compile.stdout', compiled.stdout)
                base.evidence('record-channel-compile.stderr', compiled.stderr)
                self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr), (0, b'', b''))
                base.evidence('record-channel-fixture-command.json', {'argv': [str(executable)],
                    'cwd': str(private), 'env': environment, 'timeoutSeconds': 30})
                start = time.monotonic()
                actual = subprocess.run([str(executable)], cwd=private, env=environment,
                    capture_output=True, timeout=30)
                run_seconds = time.monotonic()-start
                base.evidence('record-channel-fixture.stdout', actual.stdout)
                base.evidence('record-channel-fixture.stderr', actual.stderr)
                self.assertEqual((actual.returncode, actual.stdout, actual.stderr),
                                 (0, ORACLE.encode('ascii'), b''))
                base.evidence('record-channel-inputs.json', {
                    'sources': pins, 'orderedTranslationUnits': list(SOURCES),
                    'literalProjectInputs': 16, 'frozenSelectedInputsSha256': FROZEN,
                    'canonicalGenuineRecordingInputs': canonical_inputs,
                    'originalHelperPin': helper_before, 'wrapperPin': wrapper_before,
                    'compileSeconds': compile_seconds, 'runSeconds': run_seconds,
                    'compileTimeoutSeconds': 120, 'runTimeoutSeconds': 30,
                    'groups': 6, 'oracleBytes': ORACLE_BYTES,
                    'ownership': 'Real unchanged pt_record_input and pt_record_pattern_store, validated 16-channel project, genuine chronological pattern journal and project encode/probe/decode; no fake stores or manufactured receipts',
                    'scope': 'Portable serialized complete MIDI 1.0 channel-message adapter only. Explicit caller track/filter, NoteOn velocity-zero through genuine Off, invalid/filtered preservation, Hold Record, row63 boundary, stale releases/collisions, route/occupied-store refusal, genuine project/save/undo/redo and retained release-capacity failure. No serial/running-status/System/SysEx parser, input-source selection, CAMD resources, timestamp conversion, automatic track allocation, frontend/default activation, output, native/device quiet, exact musical timing or listening acceptance; no full system-header/toolchain closure qualification'})
                base.verify_sources(pins)
        finally:
            base.ROOT = ROOT
            base.SOURCES = old_units
            base.verify_sources(pins)
            for relative, fact in private_inputs.items():
                self.assertEqual(base.stable_fact(base.ordinary_pin(ROOT / relative)[0]), base.stable_fact(fact))
            base.ROOT = CANONICAL
            for relative, fact in canonical_inputs.items():
                self.assertEqual(base.stable_fact(base.ordinary_pin(CANONICAL / relative)[0]), base.stable_fact(fact))
            self.assertEqual(base.stable_fact(helper_before), base.stable_fact(base.ordinary_pin(HELPER)[0]))
            self.assertEqual(base.stable_fact(wrapper_before), base.stable_fact(base.ordinary_pin(Path(__file__))[0]))
            after = base.preservation_snapshot()
            base.evidence('record-channel-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
