"""Measure a known tone in a Wave-only AmiGUS recording using three logged intervals."""

import argparse
import json
import math
from pathlib import Path
import struct
import wave


def dbfs(level):
    return 20 * math.log10(level) if level else None


def measure(samples, step):
    """Fit a sine and cosine together with a constant for any starting phase."""
    size = len(samples)
    mean = sum(samples) / size
    centered = [value - mean for value in samples]
    energy = sum(value * value for value in centered)
    sine = [math.sin(step * frame) for frame in range(size)]
    cosine = [math.cos(step * frame) for frame in range(size)]
    mean_sine, mean_cosine = sum(sine) / size, sum(cosine) / size
    ss = sum((value - mean_sine) ** 2 for value in sine)
    cc = sum((value - mean_cosine) ** 2 for value in cosine)
    sc = sum((s - mean_sine) * (c - mean_cosine) for s, c in zip(sine, cosine))
    ys = sum(y * (s - mean_sine) for y, s in zip(centered, sine))
    yc = sum(y * (c - mean_cosine) for y, c in zip(centered, cosine))
    det = ss * cc - sc * sc
    if det <= 0:
        raise ValueError("The requested tone cannot be resolved in the active interval")
    fitted = ((cc * ys - sc * yc) * ys + (ss * yc - sc * ys) * yc) / det
    fitted = min(energy, max(0.0, fitted))
    return math.sqrt(fitted / size) / 32768, fitted / energy if energy else 0.0


def read_interval(recording, interval):
    start, end = interval
    count = end - start
    channels = recording.getnchannels()
    recording.setpos(start)
    data = recording.readframes(count)
    if len(data) != 2 * channels * count:
        raise ValueError("Recording ends before the logged interval")
    samples = struct.unpack(f"<{channels * count}h", data)
    return [samples[channel::channels] for channel in range(channels)]


def read_capture(path, intervals, frequency):
    with wave.open(str(path), "rb") as recording:
        rate = recording.getframerate()
        channels = recording.getnchannels()
        if recording.getsampwidth() != 2 or channels not in (1, 2):
            raise ValueError("Expected a mono or stereo 16-bit PCM WAV")
        if not 0 < frequency < rate / 2:
            raise ValueError("Expected a tone frequency below half the recording sample rate")
        before, active, after = intervals
        if not (0 <= before[0] < before[1] <= active[0] < active[1] <= after[0] < after[1]
                <= recording.getnframes()):
            raise ValueError("Expected three nonempty, ordered, disjoint intervals within the recording")
        if (active[1] - active[0]) * frequency < 2 * rate:
            raise ValueError("The active interval must include at least two periods of the expected tone")
        parts = [read_interval(recording, interval) for interval in intervals]
    return rate, parts


def check_capture(path, intervals, frequency, quiet_dbfs, active_dbfs, tone_fraction,
                  uninterrupted_reference=None):
    if not math.isfinite(quiet_dbfs) or not quiet_dbfs < active_dbfs <= 0 or not 0 < tone_fraction <= 1:
        raise ValueError("Expected a lower quiet level, a higher active level, and a valid tone fraction")
    rate, parts = read_capture(path, intervals, frequency)

    quiet_limit = 10 ** (quiet_dbfs / 20)
    min_tone = 10 ** (active_dbfs / 20)
    results = []
    for pre, playing, post in zip(*parts):
        pre_peak = max(map(abs, pre)) / 32768
        post_peak = max(map(abs, post)) / 32768
        rms, fraction = measure(playing, math.tau * frequency / rate)
        clipped = min(playing) == -32768 or max(playing) == 32767
        results.append({"before_peak_dbfs": dbfs(pre_peak), "after_peak_dbfs": dbfs(post_peak),
                        "active_tone_rms_dbfs": dbfs(rms), "active_tone_energy_fraction": fraction,
                        "active_clipped": clipped,
                        "quiet_intervals_passed": max(pre_peak, post_peak) <= quiet_limit,
                        "active_tone_detected": rms >= min_tone and fraction >= tone_fraction})
    passed = (all(r["quiet_intervals_passed"] and not r["active_clipped"] for r in results)
              and any(r["active_tone_detected"] for r in results))
    reference = None
    if uninterrupted_reference is not None:
        reference_rate, reference_parts = read_capture(uninterrupted_reference, intervals, frequency)
        if reference_rate != rate or len(reference_parts[0]) != len(results):
            raise ValueError("Expected the same sample rate and channels in both recordings")
        after_start, after_end = intervals[2]
        if (after_end - after_start) * frequency < 2 * rate:
            raise ValueError("The after interval must include at least two periods for the reference")
        reference = []
        for before, active, after in zip(*reference_parts):
            active_rms, active_fraction = measure(active, math.tau * frequency / rate)
            after_rms, after_fraction = measure(after, math.tau * frequency / rate)
            reference.append({
                "before_quiet": max(map(abs, before)) / 32768 <= quiet_limit,
                "clipped": any(min(section) == -32768 or max(section) == 32767
                               for section in (active, after)),
                "active_tone_detected": active_rms >= min_tone and active_fraction >= tone_fraction,
                "after_tone_detected": after_rms >= min_tone and after_fraction >= tone_fraction,
                "after_tone_rms_dbfs": dbfs(after_rms),
                "after_tone_energy_fraction": after_fraction,
            })
        passed = (passed and all(r["before_quiet"] and not r["clipped"] for r in reference)
                  and any(result["active_tone_detected"] and r["active_tone_detected"]
                          and r["after_tone_detected"] for result, r in zip(results, reference)))
    return {"scope": "wave-recording-only", "sample_rate_hz": rate, "expected_tone_hz": frequency,
            "intervals_frames": dict(zip(("before", "active", "after"), intervals)),
            "limits": {"quiet_peak_dbfs": quiet_dbfs, "active_tone_rms_dbfs": active_dbfs,
                       "min_active_tone_energy_fraction": tone_fraction},
            "channels": results, "uninterrupted_reference_channels": reference,
            "stop_comparison_passed": passed if reference is not None else None,
            "passed": passed, "sample_ram_quiet_qualified": False}


def interval(value):
    try:
        start, end = value.split(":")
        return int(start), int(end)
    except ValueError:
        raise argparse.ArgumentTypeError("Expected START:END frame indices") from None


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wav", type=Path)
    for name in ("before", "active", "after"):
        parser.add_argument(f"--{name}", type=interval, required=True, metavar="START:END")
    parser.add_argument("--tone-hz", type=float, required=True)
    parser.add_argument("--quiet-peak-dbfs", type=float, required=True)
    parser.add_argument("--active-tone-rms-dbfs", type=float, required=True)
    parser.add_argument("--min-tone-fraction", type=float, required=True)
    parser.add_argument("--uninterrupted-reference", type=Path,
                        help="matched recording of the same voice allowed to play past the Stop window")
    args = parser.parse_args()
    try:
        report = check_capture(args.wav, (args.before, args.active, args.after), args.tone_hz,
                               args.quiet_peak_dbfs, args.active_tone_rms_dbfs, args.min_tone_fraction,
                               args.uninterrupted_reference)
    except (OSError, EOFError, ValueError, wave.Error) as error:
        parser.exit(2, f"{error}\n")
    print(json.dumps(report, indent=2, allow_nan=False))
    raise SystemExit(0 if report["passed"] else 1)
