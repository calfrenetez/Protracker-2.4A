"""Proposed separate isolated HOST publication fixture. Not imported or run.

Compile the real private owner and genuine queue/master/cache units separately.
No source inclusion of production C, native target, timer, Git or repo mutation.
Accepted causal26, default v1 and one-armed fixtures remain separate gates.
"""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/mixed_readers_causal_publication_test.c', 'src/core/mixed_readers_causal.c',
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
ORACLE = 'MIXED CAUSAL PUBLICATION PASS:56 genuine heap cases;48 first/successor raw0/-1/malformed identity/full-copy/reentry/fault/late publication paths;independent C/R proof orders;faulting raw0 retains pins and unchanged outputs until exact later proofs;6 real allocation/reentry/fail-closed constructors;2 actual consumed-close0 release lifetimes;bound original registration;master beforeimages;SOFTWARE_ONLY\n'


class MixedCausalPublicationSoftware(unittest.TestCase):
    def test_publication_faults_and_actual_constructor_release_lifetimes(self):
        environment = dict(os.environ)
        environment['ASAN_OPTIONS'] = 'halt_on_error=1:abort_on_error=1'
        environment['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
        with tempfile.TemporaryDirectory(prefix='pt-mixed-causal-publication-') as temporary:
            executable = Path(temporary) / 'mixed-causal-publication'
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
