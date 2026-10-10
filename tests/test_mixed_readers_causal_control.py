"""HOST after-first one-CONTROL groundwork; not native scheduling acceptance.

Default two-TRIGGER fixture remains byte-identical and independently tested.
Real owner/queue/master/cache units compile separately under ASAN and UBSAN.
No target, installed infrastructure, physical capacity/timing or audio evidence.
"""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/mixed_readers_causal_control_test.c', 'src/core/mixed_readers_causal.c',
           'src/core/mixed_scheduled_readers.c', 'src/core/elapsed_clock.c',
           'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c',
           'src/core/amigus_voice_plan.c', 'src/editor/sampler.c', 'src/editor/slots.c',
           'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c',
           'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c',
           'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c',
           'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c',
           'src/core/amigus_reservation.c', 'src/core/amigus_wavetable_cache.c',
           'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c',
           'src/core/playback_pcm.c']
ORACLE = 'MIXED CAUSAL AFTER FIRST CONTROL HOST PASS:34 genuine heap cases;18 mixed4/12 or subset 8/16/24-master cache8/16 same-reader lifetimes;actual first and queue ACTIVE C/R observations;disposed poisoned first C;typed guarded internally derived keys;distinct capability and port completion;expected-only quiet;partial/zero/malformed/late/reentry/uncertain retention;actual enqueue transfer authority;master beforeimages;NO_NATIVE_APERTURE_QUALIFICATION\n'


class MixedCausalControlSoftware(unittest.TestCase):
    def test_after_first_control_and_independent_lifetimes(self):
        environment = dict(os.environ)
        environment['ASAN_OPTIONS'] = 'halt_on_error=1:abort_on_error=1'
        environment['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
        with tempfile.TemporaryDirectory(prefix='pt-mixed-causal-') as temporary:
            executable = Path(temporary) / 'mixed-causal'
            compiled = subprocess.run([
                '/usr/bin/cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-UNDEBUG', '-fsanitize=address,undefined', '-Isrc/core', '-I.',
                *SOURCES, '-o', str(executable),
            ], cwd=ROOT, env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, check=True, timeout=120)
            self.assertEqual(compiled.stdout, b'')
            self.assertEqual(compiled.stderr, b'')
            observed = subprocess.run([str(executable)], cwd=ROOT, env=environment,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True, timeout=30)
            self.assertEqual(observed.stderr, b'')
            self.assertEqual(observed.stdout, ORACLE.encode('ascii'))


if __name__ == '__main__':
    unittest.main()
