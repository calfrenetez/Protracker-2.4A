from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class VoiceAdvance(unittest.TestCase):
    def test_phase_without_pcm(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'advance'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/voice_advance_test.c','src/core/voice.c','src/core/pcm.c','-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
