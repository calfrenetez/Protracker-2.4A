"""Stable-endpoint registry HOST qualification; no CAMD/native/output timing.

Only the three selected TUs and their exact eight-body project closure are copied.
The genuine MIDI core is unchanged; recorded callbacks exercise actual ownership.
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
SOURCES = ('tests/midi_endpoints_test.c', 'src/platform/midi_endpoints.c', 'src/core/midi.c')
CFLAGS = ('-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
          '-fsanitize=address,undefined', '-Isrc/core', '-Itests', '-I.')
FROZEN = {'tests/midi_endpoints_test.c': '82a5bde919cc6d8ad7165a9988b5f9f6ee43a2931cb135ea74ee6ba389b1b683', 'src/platform/midi_endpoints.c': 'acf4bc39492a65a48940670c31566ec566969e543bed4976409416551d7c7b2e', 'src/platform/midi_endpoints.h': '89af1d6d0cd3824e825f3d6b6823bd24d5896db69e8408f004f43dabb5ea5eda', 'src/core/midi.c': '128612f5b7111e64f7656e7376a33136d985db9720cdf5b87b4797f9ac7aa28f', 'src/core/midi.h': '0d1cf918cef5a9f53ef2f097e84648be7b753f150e662953db71108025065056', 'src/core/project.h': 'e1a749800258b1f1db1dedcd7d5d36970abbf7494279cb3669212757f982e21a', 'src/core/channels.h': '9eadeee6befb0f291e62c654a8ccd5ed6c6e7030f8c8c93587566fb06eb99517', 'src/core/pcm.h': '9b6996caf7c81bd53034f53e008e7681885ede2711dddf7c2027c3bab06cb10e'}
CORE_INPUTS = ('src/core/midi.c', 'src/core/midi.h', 'src/core/project.h',
               'src/core/channels.h', 'src/core/pcm.h')
ORACLE = 'MIDI ENDPOINT HOST PASS:11 cases;stable16 identities;genuine note/sustain ownership;sendless discovery;explicit reconnect;partial STOP;serialized reentry;NO CAMD/PLAY/TIMING AUTHORITY\n'
ORACLE_BYTES = 184


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
    base = types.ModuleType('pt_midi_endpoint_frozen_helper')
    base.__file__ = str(HELPER)
    exec(compile(bytes(body), str(HELPER), 'exec'), base.__dict__)
    assert base.ROOT == CANONICAL and base.CFLAGS == CFLAGS
    return base


class MidiEndpointHost(unittest.TestCase):
    def test_stable_identity_and_genuine_release_obligations(self):
        base = load_pinned_helper()
        before = base.preservation_snapshot()
        base.evidence('midi-endpoints-before-preservation.json', before)
        helper_before, _ = base.ordinary_pin(HELPER)
        wrapper_before, _ = base.ordinary_pin(Path(__file__))
        old_units = base.SOURCES
        canonical_inputs = {}
        private_inputs = {}
        pins = {}
        try:
            self.assertEqual(len(SOURCES), 3)
            self.assertEqual(len(set(SOURCES)), 3)
            self.assertEqual(len(FROZEN), 8)
            self.assertEqual(len(ORACLE.encode('ascii')), ORACLE_BYTES)
            for relative in CORE_INPUTS:
                fact, _ = base.ordinary_pin(CANONICAL / relative)
                self.assertEqual(fact['sha256'], FROZEN[relative], 'genuine canonical MIDI dependency changed')
                canonical_inputs[relative] = fact
            for relative, sha in FROZEN.items():
                fact, _ = base.ordinary_pin(ROOT / relative)
                self.assertEqual(fact['sha256'], sha)
                private_inputs[relative] = fact
            base.ROOT = ROOT
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-midi-endpoint-host-') as temporary:
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
                base.evidence('midi-endpoints-compile-command.json', {'argv': command,
                    'cwd': str(private), 'env': environment, 'timeoutSeconds': 120})
                start = time.monotonic()
                compiled = subprocess.run(command, cwd=private, env=environment,
                    capture_output=True, timeout=120)
                compile_seconds = time.monotonic()-start
                base.evidence('midi-endpoints-compile.stdout', compiled.stdout)
                base.evidence('midi-endpoints-compile.stderr', compiled.stderr)
                self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr), (0, b'', b''))
                base.evidence('midi-endpoints-fixture-command.json', {'argv': [str(executable)],
                    'cwd': str(private), 'env': environment, 'timeoutSeconds': 30})
                start = time.monotonic()
                actual = subprocess.run([str(executable)], cwd=private, env=environment,
                    capture_output=True, timeout=30)
                run_seconds = time.monotonic()-start
                base.evidence('midi-endpoints-fixture.stdout', actual.stdout)
                base.evidence('midi-endpoints-fixture.stderr', actual.stderr)
                self.assertEqual((actual.returncode, actual.stdout, actual.stderr),
                                 (0, ORACLE.encode('ascii'), b''))
                base.evidence('midi-endpoints-inputs.json', {
                    'sources': pins, 'orderedTranslationUnits': list(SOURCES),
                    'literalProjectInputs': 8, 'frozenSelectedInputsSha256': FROZEN,
                    'canonicalGenuineCoreInputs': canonical_inputs,
                    'originalHelperPin': helper_before, 'wrapperPin': wrapper_before,
                    'compileSeconds': compile_seconds, 'runSeconds': run_seconds,
                    'compileTimeoutSeconds': 120, 'runTimeoutSeconds': 30,
                    'cases': 11, 'oracleBytes': ORACLE_BYTES,
                    'ownership': 'Real unchanged pt_midi fields and accepted complete-message callback; no fake lease counts or ledger writes',
                    'scope': 'Portable serialized HOST adapter only; exact case-sensitive names, bounded16 slots, sendless validated discovery/selection, explicit connect/reconnect, genuine partial flush/STOP and rejected callback reentry. No CAMD ABI/link lifecycle, frontend/PLAY/native runtime, musical scheduling, physical device, quiet or listening acceptance; no full system-header/toolchain closure qualification'})
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
            base.evidence('midi-endpoints-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
