"""Frozen HOST-only integration of never-exposed Paula preparation resources.

The genuine sampler, master-pin jobs, Paula upload/cache jobs and bounded fixture
allocators are linked. Two deliberately HELD reentry fixtures retain their real
owner/context allocations until process exit; clean cases retire all allocations.
No native ABI, Chip memory class, DMA, source quiet, IRQ, output or timing claim.
"""
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
import test_native_mixed_causal_rolling_entry as base

SOURCES = (
    'tests/prepared_paula_sources_test.c',
    'src/editor/prepared_paula_sources.c',
    'src/editor/sampler_paula.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c',
    'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c',
    'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c',
    'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c',
    'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c',
    'src/core/raw.c', 'src/editor/sampler_lifetime.c', 'src/core/sample_usage.c')
FROZEN = {
    'src/editor/prepared_paula_sources.c':
        '18f67c477e8bebca6cd7dea7b9928702165804cb512dfdd5748bb7f8f5d187ed',
    'src/editor/prepared_paula_sources.h':
        '5e4efbd9d250303bf5c743b6b6062f9c605072981f540918a066baacb3424152'}
ORACLE = ('PREPARED PAULA SOURCES HOST PASS:real mono8/16/24 pins/cache;'
          'load24/hit9/cancel25;save/alias/stale36/budget/retained2;NO OUTPUT AUTHORITY\n')


class PreparedPaulaSourcesHost(unittest.TestCase):
    def test_real_child_ownership_under_sanitizers(self):
        before = base.preservation_snapshot()
        base.evidence('prepared-paula-sources-before-preservation.json', before)
        old_units = base.SOURCES
        pins = {}
        try:
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-prepared-paula-sources-') as temporary:
                private = Path(temporary)
                pins = base.source_snapshot(private)
                for relative, sha in FROZEN.items():
                    self.assertEqual(pins[relative]['sha256'], sha)
                self.assertEqual(len(SOURCES), len(set(SOURCES)))
                environment = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(private),
                    'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1',
                    'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
                executable = private / 'fixture'
                deadline = time.monotonic() + 120
                compiled = subprocess.run(['/usr/bin/cc', *base.CFLAGS, *SOURCES,
                                           '-o', str(executable)], cwd=private,
                    env=environment, capture_output=True,
                    timeout=max(0.1, deadline-time.monotonic()))
                base.evidence('prepared-paula-sources-compile.stdout', compiled.stdout)
                base.evidence('prepared-paula-sources-compile.stderr', compiled.stderr)
                self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr), (0, b'', b''))
                actual = subprocess.run([str(executable)], cwd=private, env=environment,
                                        capture_output=True, timeout=30)
                base.evidence('prepared-paula-sources-fixture.stdout', actual.stdout)
                base.evidence('prepared-paula-sources-fixture.stderr', actual.stderr)
                self.assertEqual((actual.returncode, actual.stdout, actual.stderr),
                                 (0, ORACLE.encode('ascii'), b''))
                base.evidence('prepared-paula-sources-inputs.json', {
                    'sources': pins, 'orderedTranslationUnits': list(SOURCES),
                    'frozenHolderSha256': FROZEN,
                    'coverage': {'loadTransitions': 24, 'hitTransitions': 9,
                        'cancellationCutsIncludingInitAndReady': 25,
                        'staleCleanupCases': 36, 'deliberatelyRetainedHeapFixtures': 2},
                    'retainedBoundary': 'Forbidden callbacks latch HELD; genuine surviving owner/context resources remain reachable, never force-released',
                    'scope': 'HOST real software ownership integration only; no native or output authority',
                    'compileTimeoutSeconds': 120, 'runTimeoutSeconds': 30})
                base.verify_sources(pins)
        finally:
            base.SOURCES = old_units
            base.verify_sources(pins)
            after = base.preservation_snapshot()
            base.evidence('prepared-paula-sources-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
