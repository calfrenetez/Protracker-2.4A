"""Independent HOST-only ordinary-C delivery-gate model.

Fake Exec separates signed nesting from CPU IPL and can synchronously deliver
ordinary C inside Enable. This proves bounded metadata state ordering only:
never real IRQ/context legitimacy, saved SR, source lifetime, activation, timing,
output, device completion, retirement, or quiet. All project inputs are copied
with exact pins and the protected16/classic baseline remain unchanged.
"""
from pathlib import Path
import hashlib
import re
import subprocess
import tempfile
import time
import unittest
import test_native_mixed_causal_rolling_entry as base

SOURCES = ('tests/prepared_delivery_gate_test.c',
           'src/native/prepared_delivery_gate.c')
ORACLE = ('PREPARED DELIVERY GATE HOST PASS:constructor/task/local/deferred/fault;'
          'MODELED ORDINARY C ONLY\n')
FAKE = {
    'exec/tasks.h': '''#ifndef PT_TEST_EXEC_TASKS_H
#define PT_TEST_EXEC_TASKS_H
#include <stdint.h>
struct Task { void *identity; uint64_t protected_words[15]; };
#endif
''',
    'exec/execbase.h': '''#ifndef PT_TEST_EXEC_EXECBASE_H
#define PT_TEST_EXEC_EXECBASE_H
#include <stdint.h>
#include <exec/tasks.h>
typedef signed char BYTE;
struct ExecBase {
    uint64_t protected_before[8];
    BYTE IDNestCnt;
    struct Task *ThisTask;
    uint64_t protected_after[8];
};
extern struct ExecBase *SysBase;
#endif
''',
    'proto/exec.h': '''#ifndef PT_TEST_PROTO_EXEC_H
#define PT_TEST_PROTO_EXEC_H
#include <exec/execbase.h>
void Disable(void);
void Enable(void);
#endif
''',
}


class PreparedDeliveryGateHost(unittest.TestCase):
    def test_numeric_slots_and_owned_exec_pairs_under_sanitizers(self):
        before = base.preservation_snapshot()
        base.evidence('prepared-delivery-gate-before-preservation.json', before)
        original_units = base.SOURCES
        pins = {}
        try:
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-prepared-delivery-gate-') as temporary:
                private = Path(temporary)
                pins = base.source_snapshot(private)
                self.assertEqual(set(pins), {*SOURCES, 'src/native/prepared_delivery_gate.h'})
                fake_pins = {}
                for relative, text in FAKE.items():
                    path = private / 'fake' / relative
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text(text, encoding='ascii')
                    path.chmod(0o400)
                    fake_pins[relative] = hashlib.sha256(text.encode('ascii')).hexdigest()
                env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(private),
                       'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1',
                       'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
                flags = (*base.CFLAGS, '-Ifake')
                module = private / 'gate.o'
                compile_deadline = time.monotonic() + 120
                compiled = subprocess.run(['/usr/bin/cc', *flags, '-c', SOURCES[1],
                                           '-o', str(module)], cwd=private, env=env,
                                          capture_output=True,
                                          timeout=max(0.1, compile_deadline-time.monotonic()))
                base.evidence('prepared-delivery-gate-module-compile.stdout', compiled.stdout)
                base.evidence('prepared-delivery-gate-module-compile.stderr', compiled.stderr)
                self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr),
                                 (0, b'', b''))
                unresolved = subprocess.run(['/usr/bin/nm', '-u', str(module)],
                                            cwd=private, env=env, capture_output=True,
                                            timeout=30)
                self.assertEqual((unresolved.returncode, unresolved.stderr), (0, b''))
                symbols = [line.split()[-1].lstrip('_')
                           for line in unresolved.stdout.decode('ascii').splitlines()
                           if line.strip()]
                production_symbols = sorted(symbol for symbol in symbols
                                            if not symbol.startswith(('asan_', 'ubsan_')))
                self.assertEqual(set(production_symbols)-{'Disable', 'Enable', 'SysBase',
                                                         'memcpy', 'memmove', 'memset'}, set())
                self.assertTrue({'Disable', 'Enable', 'SysBase'} <= set(production_symbols))
                # Absence is checked on the actual compiled production module,
                # rather than invented fixture counters for APIs it never calls.
                self.assertFalse(any(re.search(
                    r'(Alloc|Free|malloc|calloc|realloc|free|Wait|Delay|Cause|Timer|'
                    r'ReadEClock|GetSysTime|FindTask|Write|Release|Retain|Output)', symbol)
                                     for symbol in production_symbols))
                executable = private / 'fixture'
                linked = subprocess.run(['/usr/bin/cc', *flags, SOURCES[0], str(module),
                                         '-o', str(executable)], cwd=private, env=env,
                                        capture_output=True,
                                        timeout=max(0.1, compile_deadline-time.monotonic()))
                base.evidence('prepared-delivery-gate-compile.stdout', linked.stdout)
                base.evidence('prepared-delivery-gate-compile.stderr', linked.stderr)
                self.assertEqual((linked.returncode, linked.stdout, linked.stderr), (0, b'', b''))
                actual = subprocess.run([str(executable)], cwd=private, env=env,
                                        capture_output=True, timeout=30)
                base.evidence('prepared-delivery-gate-fixture.stdout', actual.stdout)
                base.evidence('prepared-delivery-gate-fixture.stderr', actual.stderr)
                self.assertEqual((actual.returncode, actual.stdout, actual.stderr),
                                 (0, ORACLE.encode('ascii'), b''))
                for relative, sha in fake_pins.items():
                    self.assertEqual(base.ordinary_pin(private / 'fake' / relative)[0]['sha256'], sha)
                base.evidence('prepared-delivery-gate-inputs.json', {
                    'sources': pins, 'privateFakeHeaderSha256': fake_pins,
                    'productionUndefinedSymbols': production_symbols,
                    'scope': 'HOST ordinary C metadata model only; no native or IRQ qualification',
                    'compileTimeoutSeconds': 120, 'runTimeoutSeconds': 30})
                base.verify_sources(pins)
        finally:
            base.SOURCES = original_units
            base.verify_sources(pins)
            after = base.preservation_snapshot()
            base.evidence('prepared-delivery-gate-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
