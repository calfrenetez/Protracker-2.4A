"""Pure numeric containment equivalence against the original two-span check."""
from pathlib import Path
import subprocess
import tempfile
import unittest
import test_native_mixed_causal_rolling_entry as base


class PrepareSpan(unittest.TestCase):
    def test_boundaries_and_random_numeric_ranges_without_dereferences(self):
        before=base.preservation_snapshot();base.evidence('span-before-preservation.json',before)
        original_sources=base.SOURCES;pins={}
        try:
            base.SOURCES=('tests/prepare_span_test.c',)
            with tempfile.TemporaryDirectory(prefix='pt-prepare-span-') as directory:
                root=Path(directory);pins=base.source_snapshot(root);exe=root/'fixture'
                env={'PATH':'/usr/bin:/bin','LC_ALL':'C','TMPDIR':str(root),
                     'ASAN_OPTIONS':'halt_on_error=1:abort_on_error=1',
                     'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
                compiled=subprocess.run(['/usr/bin/cc',*base.CFLAGS,*base.SOURCES,'-o',str(exe)],cwd=root,env=env,capture_output=True,timeout=60)
                base.evidence('span-compile.stdout',compiled.stdout);base.evidence('span-compile.stderr',compiled.stderr)
                self.assertEqual((compiled.returncode,compiled.stdout,compiled.stderr),(0,b'',b''))
                run=subprocess.run([str(exe)],cwd=root,env=env,capture_output=True,timeout=30)
                base.evidence('span-stdout',run.stdout);base.evidence('span-stderr',run.stderr)
                self.assertEqual((run.returncode,run.stdout,run.stderr),(0,b'PREPARE SPAN EQUIVALENCE PASS\n',b''))
                base.verify_sources(pins)
        finally:
            base.SOURCES=original_sources;base.verify_sources(pins)
            after=base.preservation_snapshot();base.evidence('span-after-preservation.json',after)
            self.assertEqual(base.stable_fact(before),base.stable_fact(after))


if __name__=='__main__':unittest.main()
