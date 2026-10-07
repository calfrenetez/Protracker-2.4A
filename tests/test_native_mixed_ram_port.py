"""Standalone genuine mixed RAM port HOST test; source proposal until selected.

The actual new native_mixed_ram_port.c is compiled separately. The old resource
fixture entry is renamed and not executed. No CIA, emulator or hardware action.
"""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/native_mixed_ram_port_test.c', 'src/native/readers_ram/native_mixed_ram_port.c', 'src/core/mixed_readers_activation.c', 'src/core/mixed_scheduled_readers.c', 'src/core/elapsed_clock.c', 'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/core/amigus_reservation.c', 'src/core/amigus_wavetable_cache.c', 'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c']


class NativeMixedRamPortSoftware(unittest.TestCase):
    def test_actual_genuine_domains_and_original_windows(self):
        environment = dict(os.environ)
        environment['ASAN_OPTIONS'] = 'halt_on_error=1:abort_on_error=1'
        environment['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
        with tempfile.TemporaryDirectory(prefix='pt-mixed-ram-port-') as temporary:
            executable = Path(temporary) / 'native-mixed-ram-port'
            subprocess.run([
                '/usr/bin/cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra',
                '-Werror', '-UNDEBUG', '-fsanitize=address,undefined',
                '-Isrc/core', '-Isrc/native/readers_ram', '-I.',
                *SOURCES, '-o', str(executable),
            ], cwd=ROOT, env=environment, check=True, timeout=120)
            subprocess.run([str(executable)], cwd=ROOT, env=environment, check=True, timeout=30)


if __name__ == '__main__':
    unittest.main()
