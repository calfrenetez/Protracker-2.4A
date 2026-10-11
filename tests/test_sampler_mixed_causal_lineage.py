"""Host assertions for the finite TRIGGER, CONTROL and STOP sampler factory.

Native scheduling, peripheral stop and process-group custody remain separate.
"""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/sampler_mixed_causal_lineage_test.c',
           'src/editor/sampler_mixed_readers.c', 'src/core/amigus_trigger_levels.c',
           'src/core/mixed_readers_causal.c', 'src/core/mixed_scheduled_readers.c',
           'src/core/elapsed_clock.c', 'src/editor/sampler_paula.c',
           'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c',
           'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c',
           'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c',
           'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c',
           'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c',
           'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c',
           'src/core/amigus_reservation.c', 'src/core/amigus_wavetable_cache.c',
           'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c',
           'src/core/playback_pcm.c']
ORACLE = ('SAMPLER CAUSAL LINEAGE FACTORY HOST COMPLETE cases=74;'
          '24 mixed4/12 or16card 8/16/24-master cache8/16 lifetimes;'
          'exact960/1920/2880;two live C and32R bound;'
          'actual C1 service and NULL close;disposed opaque C/token reuse;'
          'CONTROL/STOP zero new R/pin/cache/upload;complete typed guards;'
          'consumed allocator attempts;publication uncertainty;'
          'independent C/R/SOURCE;full actual-capacity masters and exact save/cache beforeimages;'
          'NOT_NATIVE_CONTROLLER_DEVICE_AUDIO\n')


class SamplerCausalLineageSoftware(unittest.TestCase):
    def test_actual_trigger_control_stop_lifetimes(self):
        with tempfile.TemporaryDirectory(prefix='pt-sampler-causal-lineage-') as temporary:
            executable = Path(temporary) / 'sampler-causal-lineage'
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
