from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
# Public-only shared fixture include; all 31 production implementations are
# separate translation units. No implementation C is included in the fixture.
ACTIVATION_SOURCES = [
    'tests/readers_activation_test.c',
    'src/core/readers_activation.c',
    'src/editor/sampler_paula.c',
    'src/core/sample_cache.c',
    'src/core/playback_pcm.c',
    'src/core/elapsed_clock.c',
    'src/core/scheduled_readers.c',
    'src/core/paula_render_voice.c',
    'src/editor/sampler.c',
    'src/editor/slots.c',
    'src/core/pcm_filtered.c',
    'src/core/slices.c',
    'src/core/pattern.c',
    'src/core/document.c',
    'src/core/pp20.c',
    'src/core/project.c',
    'src/core/mod_project.c',
    'src/core/mod_inspect.c',
    'src/core/channels.c',
    'src/core/pcm.c',
    'src/core/wav.c',
    'src/core/svx.c',
    'src/core/raw.c',
    'src/editor/paula_preflight.c',
    'src/core/render.c',
    'src/core/pitch.c',
    'src/core/timeline.c',
    'src/core/frame_clock.c',
    'src/core/flow.c',
    'src/core/voice.c',
    'src/editor/sampler_paula_readers.c',
    'src/editor/paula_readers_song.c',
]



class ReadersActivation(unittest.TestCase):
    def test_genuine_copied_activation_and_independent_domains(self):
        with tempfile.TemporaryDirectory() as tmp:
            executable = Path(tmp) / 'test'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-Isrc/core',
                *ACTIVATION_SOURCES, '-o', str(executable),
            ], cwd=ROOT, check=True)
            subprocess.run([str(executable)], check=True)
