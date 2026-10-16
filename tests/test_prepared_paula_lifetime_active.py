"""Frozen HOST-only adopted-reader lifetime after genuine publication.

The actual sampler/cache owners and readers queue are separately compiled.
A serialized HOST model dispatches at synthetic2184, then uses persistent-R
geometry independently of the command event. Eight cases complete5 and retain3.
No output, real timing, native ABI, device, hardware quiet or physical claim.
"""
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
import test_native_mixed_causal_rolling_entry as base

SOURCES = (
    'tests/prepared_paula_lifetime_active_test.c',
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
        'b2f5736eb7fc6a7a6362d72421459162399b3cf0732fd14c5a40942339860e9f',
    'src/editor/prepared_paula_sources.h':
        'ad5069642d6ae242264353f2466043feb6ab51f0f7e7972b2838d9cb01d2c71c',
    'src/editor/prepared_paula_lifetime.c':
        '752eba455a1ea973aecf5b7eef1a4cf1852578bd3218b44e55a893e5cb80ed60',
    'src/editor/prepared_paula_lifetime.h':
        '122a19963a54d4d6dfaf1f18183f342ddf2c2a8a8b1b4e547d16fb2c1857148e',
    'tests/prepared_paula_lifetime_active_test.c':
        '8074483c516ea5bd9e4b1ca746d0149acd4d69758fd5718b17bc38091bfab869',
    'tests/prepared_paula_lifetime_test.c':
        'a98ff044aef08c9f84bb21d24a6c966610cc2d91ff5be5bd5425224b02c4e3f3'}
ORACLE = ('PREPARED PAULA ACTIVE HOST PASS:8 cases;5 completed;3 retained;'
          'synthetic2184 adoption;C-first/R-first ADOPTED retirement;first-cache1026B;'
          'C-first read after detach;3 genuine source pins;deferred drain;'
          'NO OUTPUT OR TIMING AUTHORITY\n')


class PreparedPaulaLifetimeActiveHost(unittest.TestCase):
    def test_actual_model_adoption_and_independent_retirement_under_sanitizers(self):
        before = base.preservation_snapshot()
        base.evidence('prepared-paula-active-before-preservation.json', before)
        old_units = base.SOURCES
        pins = {}
        try:
            self.assertEqual(len(SOURCES), 25)
            self.assertEqual(len(set(SOURCES)), 25)
            self.assertEqual(len(ORACLE.encode('ascii')), 231)
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-prepared-paula-active-') as temporary:
                private = Path(temporary)
                # The literal donor include is frozen helper input, not a
                # separate TU; its renamed main/cases/oracle never execute.
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
                base.evidence('prepared-paula-active-compile.stdout', compiled.stdout)
                base.evidence('prepared-paula-active-compile.stderr', compiled.stderr)
                self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr),
                                 (0, b'', b''))
                actual = subprocess.run([str(executable)], cwd=private, env=environment,
                                        capture_output=True, timeout=30)
                base.evidence('prepared-paula-active-fixture.stdout', actual.stdout)
                base.evidence('prepared-paula-active-fixture.stderr', actual.stderr)
                self.assertEqual((actual.returncode, actual.stdout, actual.stderr),
                                 (0, ORACLE.encode('ascii'), b''))
                base.evidence('prepared-paula-active-inputs.json', {
                    'sources': pins, 'orderedTranslationUnits': list(SOURCES),
                    'additionalHostFlags': list(HOST_FLAGS),
                    'frozenProductionFixtureAndDonorSha256': FROZEN,
                    'coverage': {'cases': 8, 'completed': 5, 'retained': 3,
                        'cleanReceiptOrders': 2, 'staleAfterActualAdoption': 2,
                        'postCommandDetachBadReaderClassification': 1,
                        'postCommandDetachPendingOnceRetained': 1,
                        'postCommandDetachWrongReaderEnvelopeRetained': 1,
                        'postCommandDetachUncertainRetained': 1,
                        'modelDispatches': 8, 'firstCacheReadBytesPerCase': 1026},
                    'donorBoundary': 'Exact committed acquisition helper body included with renamed main; old cases/counters/oracle never execute or count',
                    'modelBoundary': 'Independent serialized HOST dispatch at original synthetic2184 in unchanged frame100/window2184..2205; actual persistent R geometry survives C detach; only first TRIGGER cache consumed while all three dependency pins stay held',
                    'retainedBoundary': 'Three complete fixtures and genuine source/queue/context anchors stay reachable; PENDING/UNCERTAIN retain actual active R; wrong envelope drops model R but preserves unconsumed core ownership; no retry, force cleanup or generation restoration',
                    'observationBoundary': 'C-first modeled ACTIVE and R-first ADOPTED+RETIRED receipts differ honestly; no claim core exposed ACTIVE/key permission in R-first; actual deferred disposal observed through public APIs rather than private C/R flags',
                    'scope': 'Ordinary HOST model only; no output, real clock/musical timing, native ABI, IRQ, device/quiet/physical/listening authority or full system-header/dyld audit',
                    'compileTimeoutSeconds': 120, 'runTimeoutSeconds': 30})
                base.verify_sources(pins)
        finally:
            base.SOURCES = old_units
            base.verify_sources(pins)
            after = base.preservation_snapshot()
            base.evidence('prepared-paula-active-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
