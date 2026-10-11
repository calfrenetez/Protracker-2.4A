"""Host assertions for the finite composite factory's actual ownership queries.

No native scheduling, peripheral stop, or process-group custody is qualified.
"""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/mixed_readers_causal_factory_lineage_test.c',
           'src/core/mixed_readers_causal.c',
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
ORACLE = ('MIXED CAUSAL COMPOSITE FACTORY QUERIES HOST PASS;actual installed capability;'
          'actual first and CONTROL completion;clean C1 detach;'
          'wrong capability/context/registration/reentry/fault refusal;'
          'no new reader/cache/upload;original frames;independent cleanup;'
          'SOURCE_OWNER_MODEL_ONLY_NOT_NATIVE_AUDIO\n')


class MixedCausalFactoryLineageSoftware(unittest.TestCase):
    def test_actual_capability_completions_and_detachment(self):
        with tempfile.TemporaryDirectory(prefix='pt-mixed-factory-lineage-') as temporary:
            executable = Path(temporary) / 'mixed-factory-lineage'
            environment = dict(os.environ, TMPDIR=temporary,
                               ASAN_OPTIONS='halt_on_error=1:abort_on_error=1',
                               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
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
