from pathlib import Path
import subprocess
import tempfile
import unittest

from test_mixed_readers_causal import SOURCES as AUDIO_SOURCES


ROOT = Path(__file__).resolve().parents[1]
COMBINED_PASS = 'Combined schedule: exact Paula, AmiGUS and MIDI admission, all owned Stop keys and independent partial receipts; SOFTWARE_ONLY\n'
TRACKER_PASS = 'Routed sampler boundary: exact Stop, retrigger and stale preparation discarded before publication; SOFTWARE_ONLY\n'
ACTIVATION_PASS = 'Routed activation: shared frame zero and exact Stop for six audio readers and two MIDI destinations; SOFTWARE_ONLY\n'
ACTIVATION_UNITS = ['src/core/mixed_readers_activation.c']
TRACKER_UNITS = [
    'src/core/render.c', 'src/core/pitch.c', 'src/core/timeline.c', 'src/core/frame_clock.c',
    'src/core/flow.c', 'src/core/voice.c', 'src/core/paula_render_voice.c',
    'src/core/amigus_render_voice.c', 'src/core/amigus_trigger_levels.c',
    'src/editor/mixed_readers_plan_normalize.c', 'src/editor/routed_preflight.c', 'src/core/routed_playback.c',
    'src/editor/routed_sampler_boundary.c',
    'src/editor/sampler_mixed_readers.c', 'src/core/mixed_readers_causal.c',
]


class RoutedMidiCombined(unittest.TestCase):
    def test_one_publication_and_independent_terminal_proofs(self):
        self.run_fixture('routed_midi_combined_test', [], COMBINED_PASS)

    def test_tracker_forecast_uses_the_same_schedule_for_attack_control_and_stop(self):
        self.run_fixture('routed_tracker_playback_test', TRACKER_UNITS, TRACKER_PASS)

    def test_activation_owner_publishes_frame_zero_and_owned_stop_once(self):
        self.run_fixture('routed_activation_test', ACTIVATION_UNITS, ACTIVATION_PASS, activation_regression=True)

    def run_fixture(self, fixture, extra, expected, activation_regression=False):
        with tempfile.TemporaryDirectory() as temporary:
            binary = Path(temporary) / fixture
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                '-fsanitize=address,undefined', '-Isrc/core', '-I.',
                f'tests/{fixture}.c', *AUDIO_SOURCES[2:],
                'src/core/routed_midi.c', 'src/core/midi.c', *extra, '-o', str(binary),
            ], cwd=ROOT, check=True, timeout=120)
            run = subprocess.run([str(binary)], cwd=ROOT, capture_output=True, text=True, timeout=30)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertEqual(run.stderr, '')
            if activation_regression:
                activation_log, output = run.stdout.split('\n', 1)
                self.assertTrue(activation_log.startswith('MIXED ACTIVATION PASS:'))
            else:
                output = run.stdout
            self.assertEqual(output, expected)
