from pathlib import Path
import subprocess
import tempfile
import unittest

from test_mixed_readers_causal import SOURCES as AUDIO_SOURCES


ROOT = Path(__file__).resolve().parents[1]
COMBINED_PASS = 'Combined schedule: exact Paula, AmiGUS and MIDI admission, partial receipts and owned Stop; SOFTWARE_ONLY\n'


class RoutedMidiCombined(unittest.TestCase):
    def test_one_publication_and_independent_terminal_proofs(self):
        with tempfile.TemporaryDirectory() as temporary:
            binary = Path(temporary) / 'combined'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                '-fsanitize=address,undefined', '-Isrc/core', '-I.',
                'tests/routed_midi_combined_test.c', *AUDIO_SOURCES[2:],
                'src/core/routed_midi.c', 'src/core/midi.c', '-o', str(binary),
            ], cwd=ROOT, check=True, timeout=120)
            run = subprocess.run([str(binary)], cwd=ROOT, capture_output=True, text=True, timeout=30)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertEqual(run.stderr, '')
            self.assertEqual(run.stdout, COMBINED_PASS)
