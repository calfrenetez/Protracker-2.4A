import math
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory
import unittest
import wave

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from check_amigus_wave_capture import check_capture


RATE = 8000
TONE = 637
WINDOWS = ((0, 400), (400, 800), (800, 1200))


def tone(frequency, frame, level=5000):
    return round(level * math.sin(math.tau * frequency * frame / RATE + 0.7))


class AmiGUSWaveCapture(unittest.TestCase):
    def check(self, render, channels=1, windows=WINDOWS):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "wave.wav"
            with wave.open(str(path), "wb") as output:
                output.setparams((channels, 2, RATE, 0, "NONE", "PCM"))
                output.writeframes(b"".join(struct.pack(f"<{channels}h", *render(i)) for i in range(1200)))
            return check_capture(path, windows, TONE, -55, -35, 0.85)

    def test_tone_with_independent_left_and_right_phase(self):
        def recording(i):
            if 400 <= i < 800:
                value = tone(TONE, i)
                return value + 20, -value + 20
            return 0, 0

        report = self.check(recording, 2)
        self.assertTrue(report["passed"])
        self.assertFalse(report["sample_ram_quiet_qualified"])
        self.assertTrue(all(channel["active_tone_energy_fraction"] > 0.999 for channel in report["channels"]))

    def test_unexpected_tone_or_dc_refuses(self):
        for active in (lambda i: tone(900, i), lambda i: 2000):
            with self.subTest(active=active):
                report = self.check(lambda i: (active(i) if 400 <= i < 800 else 0,))
                self.assertFalse(report["passed"])
                self.assertFalse(report["channels"][0]["active_tone_detected"])

    def test_late_signal_on_other_channel_and_clipping_refuse(self):
        late = self.check(lambda i: (tone(TONE, i) if 400 <= i < 800 else 0,
                                    200 if 800 <= i < 1200 else 0), 2)
        self.assertFalse(late["passed"])
        self.assertTrue(late["channels"][0]["active_tone_detected"])
        self.assertFalse(late["channels"][1]["quiet_intervals_passed"])

        clipped = self.check(lambda i: (32767 if i == 450 else tone(TONE, i) if 400 <= i < 800 else 0,))
        self.assertFalse(clipped["passed"])
        self.assertTrue(clipped["channels"][0]["active_clipped"])

    def test_overlapping_and_out_of_range_intervals_refuse(self):
        for windows in (((0, 500), (400, 800), (800, 1200)),
                        ((0, 400), (400, 800), (800, 1201)),
                        ((0, 400), (400, 405), (800, 1200))):
            with self.subTest(windows=windows), self.assertRaises(ValueError):
                self.check(lambda i: (0,), windows=windows)


if __name__ == "__main__":
    unittest.main()
