"""Frozen HOST-only unpublished ownership bridge for three genuine Paula sources.

The sampler, selective cache jobs and actual readers queue are linked as separate
translation units. The fixture performs no publication or backend callback.
Six complete contexts cleanly retire; two complete fault contexts remain
reachable until process exit. No native, reader quiet, DMA, device or timing claim.
"""
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
import test_native_mixed_causal_rolling_entry as base

SOURCES = (
    'tests/prepared_paula_lifetime_test.c',
    'src/editor/prepared_paula_sources.c',
    'src/editor/sampler_paula.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c',
    'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c',
    'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c',
    'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c',
    'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c',
    'src/core/raw.c', 'src/editor/sampler_lifetime.c', 'src/core/sample_usage.c',
    'src/editor/prepared_paula_lifetime.c', 'src/core/scheduled_readers.c',
    'src/core/elapsed_clock.c')
HOST_FLAGS = ('-DPT_PREPARED_PAULA_ENABLE_HOST_LIFETIME=1',)
FROZEN = {
    'src/editor/prepared_paula_sources.c':
        'f0f40f3100de1691f40ab37b725ad11e1008a6619c49e7b227a1f21658703112',
    'src/editor/prepared_paula_sources.h':
        '74a070e4d7b4e8463afb61e3d182913872a7726ca61f4fdb7d5084bbe764757c',
    'src/editor/prepared_paula_lifetime.c':
        '77e91ab9f056182faf64a04674c54db9f84436b971724336f0e35724b9da0748',
    'src/editor/prepared_paula_lifetime.h':
        '7131371ab2359a7464e638ed4fdc40014edc7cfdbc3dd8297ae4919d66915561',
    'tests/prepared_paula_lifetime_test.c':
        'a98ff044aef08c9f84bb21d24a6c966610cc2d91ff5be5bd5425224b02c4e3f3'}
ORACLE = ('PREPARED PAULA LIFETIME HOST PASS:real mono8/16/24;'
          'unpublished 1C1R local C-then-R;bounded drain/save/aliases/stale/'
          'preclaim/reentry;retained2;NO OUTPUT AUTHORITY\n')


class PreparedPaulaLifetimeHost(unittest.TestCase):
    def test_unpublished_real_queue_ownership_under_sanitizers(self):
        before = base.preservation_snapshot()
        base.evidence('prepared-paula-lifetime-before-preservation.json', before)
        old_units = base.SOURCES
        pins = {}
        try:
            self.assertEqual(len(SOURCES), 25)
            self.assertEqual(len(set(SOURCES)), 25)
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-prepared-paula-lifetime-') as temporary:
                private = Path(temporary)
                # The existing helper copies bytes and literal quoted includes;
                # it neither imports nor runs any selected source body.
                pins = base.source_snapshot(private)
                self.assertTrue(set(SOURCES) <= set(pins))
                for relative, sha in FROZEN.items():
                    self.assertEqual(pins[relative]['sha256'], sha)
                environment = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(private),
                    'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1',
                    'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
                executable = private / 'fixture'
                deadline = time.monotonic() + 120
                compiled = subprocess.run(['/usr/bin/cc', *base.CFLAGS, *HOST_FLAGS,
                                           *SOURCES, '-o', str(executable)], cwd=private,
                    env=environment, capture_output=True,
                    timeout=max(0.1, deadline-time.monotonic()))
                base.evidence('prepared-paula-lifetime-compile.stdout', compiled.stdout)
                base.evidence('prepared-paula-lifetime-compile.stderr', compiled.stderr)
                self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr),
                                 (0, b'', b''))
                actual = subprocess.run([str(executable)], cwd=private, env=environment,
                                        capture_output=True, timeout=30)
                base.evidence('prepared-paula-lifetime-fixture.stdout', actual.stdout)
                base.evidence('prepared-paula-lifetime-fixture.stderr', actual.stderr)
                self.assertEqual((actual.returncode, actual.stdout, actual.stderr),
                                 (0, ORACLE.encode('ascii'), b''))
                base.evidence('prepared-paula-lifetime-inputs.json', {
                    'sources': pins, 'orderedTranslationUnits': list(SOURCES),
                    'additionalHostFlags': list(HOST_FLAGS),
                    'frozenProductionAndFixtureSha256': FROZEN,
                    'coverage': 'Genuine mono8/16/24 masters and selective caches; unpublished 1C1R local C-then-R stop; bounded deferred disposal, save preservation, aliases, stale/preclaim/reentry and two retained complete fault contexts',
                    'retainedBoundary': 'Uncertain contexts stay reachable; no forced cleanup, failed release retry, counter repair or restored former source',
                    'scope': 'HOST unpublished software ownership only; no publication, backend/output, quiet, native, physical or timing authority',
                    'compileTimeoutSeconds': 120, 'runTimeoutSeconds': 30})
                base.verify_sources(pins)
        finally:
            base.SOURCES = old_units
            base.verify_sources(pins)
            after = base.preservation_snapshot()
            base.evidence('prepared-paula-lifetime-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
