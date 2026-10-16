"""HOST-only equivalence after the original nonempty first-span proof.

The modeled 32-bit domain does not qualify a native ABI or device. The helper
is exercised through the actual HOST pointer ABI without dereferencing either
numeric pointer, and only after the original first-span predicate succeeds.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest
import test_native_mixed_causal_rolling_entry as base


ORACLE=(b'PREPARE VALID SPAN APART PASS:3663138 tuples;2387186 valid-first;'
        b'837705 accept;1549481 refuse;1275952 invalid-first;'
        b'MODELED32/64;HOST predicate only\n')


class PrepareValidSpanApart(unittest.TestCase):
    def test_valid_first_specialization_without_pointer_dereferences(self):
        before=base.preservation_snapshot()
        base.evidence('valid-span-apart-before-preservation.json',before)
        original_sources=base.SOURCES;pins={}
        try:
            base.SOURCES=('tests/prepare_valid_span_apart_test.c',)
            with tempfile.TemporaryDirectory(prefix='pt-prepare-valid-span-apart-') as directory:
                root=Path(directory);pins=base.source_snapshot(root);exe=root/'fixture'
                base.evidence('valid-span-apart-source-pins.json',pins)
                env={'PATH':'/usr/bin:/bin','LC_ALL':'C','TMPDIR':str(root),
                     'ASAN_OPTIONS':'halt_on_error=1:abort_on_error=1',
                     'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
                compiled=subprocess.run(['/usr/bin/cc',*base.CFLAGS,*base.SOURCES,'-o',str(exe)],
                    cwd=root,env=env,capture_output=True,timeout=60)
                base.evidence('valid-span-apart-compile.stdout',compiled.stdout)
                base.evidence('valid-span-apart-compile.stderr',compiled.stderr)
                self.assertEqual((compiled.returncode,compiled.stdout,compiled.stderr),(0,b'',b''))
                run=subprocess.run([str(exe)],cwd=root,env=env,capture_output=True,timeout=30)
                base.evidence('valid-span-apart-stdout',run.stdout)
                base.evidence('valid-span-apart-stderr',run.stderr)
                self.assertEqual((run.returncode,run.stdout,run.stderr),(0,ORACLE,b''))
                base.verify_sources(pins)
        finally:
            base.SOURCES=original_sources;base.verify_sources(pins)
            after=base.preservation_snapshot()
            base.evidence('valid-span-apart-after-preservation.json',after)
            self.assertEqual(base.stable_fact(before),base.stable_fact(after))


if __name__=='__main__':unittest.main()
