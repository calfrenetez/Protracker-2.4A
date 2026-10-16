"""Frozen HOST-only published cancellation of three genuine Paula sources.

The actual readers queue and sampler/cache owners are separately compiled. The
model retains the original submitted event/domain, then drops its actual pending
consumer references before receipts.17 cases complete11 and retain6 full contexts.
No ACTIVE/adoption/issued timing, output, native, device, quiet or physical claim.
"""
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
import test_native_mixed_causal_rolling_entry as base

SOURCES = (
    'tests/prepared_paula_lifetime_published_test.c',
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
    'tests/prepared_paula_lifetime_published_test.c':
        '4b08292999178ced98fec94a5ea0fd3d8681bfd466cbfdd7430c6b85210f5548',
    'tests/prepared_paula_lifetime_test.c':
        'a98ff044aef08c9f84bb21d24a6c966610cc2d91ff5be5bd5425224b02c4e3f3'}
ORACLE = ('PREPARED PAULA PUBLISHED HOST PASS:17 cases;11 completed;6 retained;'
          'actual submit/cancel C-first/R-first;fresh publication/refusal/late guards;'
          'unchanged frame/window;mono8/16/24 held through deferred drain;'
          'sticky semantic failure;NO OUTPUT AUTHORITY\n')


class PreparedPaulaLifetimePublishedHost(unittest.TestCase):
    def test_actual_publication_and_once_only_cancel_under_sanitizers(self):
        before = base.preservation_snapshot()
        base.evidence('prepared-paula-published-before-preservation.json', before)
        old_units = base.SOURCES
        pins = {}
        try:
            self.assertEqual(len(SOURCES), 25)
            self.assertEqual(len(set(SOURCES)), 25)
            self.assertEqual(len(ORACLE.encode('ascii')), 250)
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-prepared-paula-published-') as temporary:
                private = Path(temporary)
                # Literal includes pull in the frozen donor as a helper body;
                # its renamed main is never invoked or separately compiled.
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
                base.evidence('prepared-paula-published-compile.stdout', compiled.stdout)
                base.evidence('prepared-paula-published-compile.stderr', compiled.stderr)
                self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr),
                                 (0, b'', b''))
                actual = subprocess.run([str(executable)], cwd=private, env=environment,
                                        capture_output=True, timeout=30)
                base.evidence('prepared-paula-published-fixture.stdout', actual.stdout)
                base.evidence('prepared-paula-published-fixture.stderr', actual.stderr)
                self.assertEqual((actual.returncode, actual.stdout, actual.stderr),
                                 (0, ORACLE.encode('ascii'), b''))
                base.evidence('prepared-paula-published-inputs.json', {
                    'sources': pins, 'orderedTranslationUnits': list(SOURCES),
                    'additionalHostFlags': list(HOST_FLAGS),
                    'frozenProductionFixtureAndDonorSha256': FROZEN,
                    'coverage': {'cases': 17, 'completed': 11, 'retained': 6,
                        'cleanReceiptOrders': 2, 'staleAfterPublication': 2,
                        'badCommandClassification': 2, 'badReaderClassification': 2,
                        'pendingOnceRetained': 2, 'wrongEnvelopeRetained': 2,
                        'backendReentryRetained': 2, 'freshStage2StaleRefusal': 1,
                        'submitZeroWithoutReferences': 1, 'clockExactlyFirstLate': 1},
                    'donorBoundary': 'Exact committed helper body included with renamed main; old cases/counters/oracle never execute or count',
                    'retainedBoundary': 'Six complete fixtures and genuine source/queue/context anchors stay reachable; no failed operation retry, forced cleanup or former-generation restoration',
                    'observationBoundary': 'Receipt order and deferred resource disposal observed through public APIs; no direct internal C/R-consumed flag or callback trace claim',
                    'scope': 'HOST cancellation-before-activation model only; no ACTIVE/adoption/issued timing, output, native/device/quiet/physical/listening authority or full system-header/dyld audit',
                    'compileTimeoutSeconds': 120, 'runTimeoutSeconds': 30})
                base.verify_sources(pins)
        finally:
            base.SOURCES = old_units
            base.verify_sources(pins)
            after = base.preservation_snapshot()
            base.evidence('prepared-paula-published-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
