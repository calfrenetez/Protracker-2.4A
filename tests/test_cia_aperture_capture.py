"""Saved-data HOST fixtures; synthetic clocks do not qualify a target."""
from pathlib import Path
import re
import json
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import cia_aperture_capture as c
import native_fixture_capture as ordinary


def report(*, frequency=709379, failed=False, post=False, partial=False, priority=0):
    rows = []
    count = 1 if partial else 16
    for i in range(count):
        first = 100000 + ((i + 1) * 1536 * frequency + 47999) // 48000
        last = 100000 + (((i + 1) * 1536 + 1) * frequency + 47999) // 48000
        mode = 0 if i < 12 else i - 11
        good = mode == 0 and not failed and not partial
        commit = good or (post and i == 0)
        result = 1 if good else (9 if commit else (5 if mode in (0, 3) else 3))
        after = first + 2 if good else last
        r = dict(index=i, mode=mode, phase=6, prepared=1, arm_valid=1, arm_after_valid=1,
                 delivered=1, closed=1, result=result, first=first, last=last,
                 arm_target=last + 128 if mode == 3 else first - 128,
                 arm_observed=(last + 128 if mode == 3 else first - 128) - 100,
                 arm_after=first - 64, arm_frequency=(frequency, frequency), arm_read_valid=1,
                 arm_reads=1, count=100, entry=first - 32, before=first,
                 after=after if commit else 0, last_observed=after, last_read=after,
                 last_read_frequency=frequency, last_read_valid=1,
                 valid=(1, 1, 1 if commit else 0), reads=2, commits=int(commit),
                 shadow=c.SHADOW if commit else 0, aperture_span_valid=1,
                 aperture_span=after - first + 32, dispatch_span_valid=1,
                 dispatch_before=first - 40, dispatch_after=last + 8,
                 dispatch_span=last - first + 48, dispatch_frequency=(frequency, frequency),
                 wake=4096, calls=i + 1)
        fields = []
        for k, v in r.items():
            if k in c.WIDE:
                v = f"{v >> 32}:{v & 0xffffffff}"
            elif k in c.PAIRS:
                v = "/".join(map(str, v))
            fields.append(f"{k}={v}")
        rows.append("CIA APERTURE OBS " + " ".join(fields))
    lines = [c.MODEL, "EXEC MEMORY pool_limit=10000000 reserve=262144 flags=4"]
    if partial:
        lines.append("CIA APERTURE FAIL line=149 completed=1")
    lines += rows
    lines.append("CIA APERTURE CLOSED acquired=1 chip=1 bit=0 vector_removed=1 timer_stopped=1 "
                 f"task_identity=1 priority={priority} unchanged=1 signals_restored=1 attempted={count} "
                 f"completed={count} positive={0 if failed or partial else 12} "
                 f"negative={0 if partial else 4} failures={0 if partial or not failed else 12} "
                 f"frequency={frequency} early_ticks=128 residency_ticks=256 reads_cap=64")
    lines.append("EXEC MEMORY PASS: 2 Fast allocations, zero owned bytes, budget refusal without Chip fallback")
    if not failed and not partial:
        lines.append(c.PASS)
    return ("\n".join(lines) + "\n").encode("ascii")


class ApertureCapture(unittest.TestCase):
    def test_complete_success_at_both_counter_frequencies(self):
        for f in (709379, 715909):
            result = c.validate(report(frequency=f, priority=-5), 0)
            self.assertEqual(result["qualification"], "PASSED")
            self.assertEqual(result["originalEpoch"], 100000)
            self.assertEqual(result["summary"]["priority"], -5)
            self.assertEqual(len(result["observations"]), 16)
            self.assertEqual(json.loads(json.dumps(result)), result)

    def test_failed_measurements_remain_failed_with_exact_rc_and_raw_values(self):
        for post in (False, True):
            data = report(failed=True, post=post)
            result = c.validate(data, 20)
            self.assertEqual(result["qualification"], "FAILED")
            self.assertEqual(result["returnCode"], 20)
            self.assertEqual(result["observations"][0]["commits"], int(post))
            self.assertEqual(result["observations"][0]["result"], 9 if post else 5)
            with self.assertRaises(ValueError):
                c.validate(data, 0)

    def test_partial_closed_report_retains_missing_measurements(self):
        result = c.validate(report(partial=True), 20)
        self.assertEqual(result["qualification"], "FAILED")
        self.assertEqual(len(result["observations"]), 1)
        self.assertEqual(result["summary"]["completed"], 1)
        self.assertEqual(result["failedCheck"], {"line": 149, "completed": 1})

    def test_no_relaxation_of_ordinary_rc0_or_exact_output(self):
        prefix = "PT0123-"
        raw = ("[OK]\n" + prefix + "BEFORE\n" + prefix + "STACK-QUERY-BEGIN\n"
               "Current stack size is 262144 bytes\n" + prefix + "STACK-QUERY-END\n"
               + prefix + "NATIVE-BEGIN\n" + prefix + "NATIVE-RC 20\n" + prefix + "NATIVE-END\n")
        self.assertEqual(c.controls(raw, 262144, prefix)["returnCode"], 20)
        with self.assertRaises(ValueError):
            ordinary.parse_control(raw, 262144, prefix)
        for rc in (-1, 1, 5, 10, 21, 255):
            with self.subTest(rc=rc), self.assertRaises(ValueError):
                c.controls(raw.replace("NATIVE-RC 20", f"NATIVE-RC {rc}"), 262144, prefix)
        with self.assertRaises(ValueError):
            c.controls(raw, 65536, prefix)

    def test_rejects_missing_duplicate_unknown_and_out_of_order_fields(self):
        source = report()
        obs = next(line for line in source.splitlines(keepends=True) if b"OBS " in line)
        for bad in (obs.replace(b"index=0 ", b""), obs.replace(b"index=0", b"index=1"),
                    obs.replace(b"index=0", b"index=0 index=0"), obs.replace(b"index=0", b"other=0"),
                    obs.replace(b"index=0", b"index=00"), obs.replace(b"first=0:", b"first=4294967296:"),
                    obs.replace(b"closed=1", b"closed=0"), obs.replace(b"reads=2", b"reads=65"),
                    obs.replace(b"commits=1", b"commits=0"), obs.replace(b"shadow=1095779666", b"shadow=3")):
            with self.subTest(bad=bad[:80]), self.assertRaises(ValueError):
                c.validate(source.replace(obs, bad, 1), 0)
        for line in source.splitlines(keepends=True):
            with self.subTest(line=line[:60]), self.assertRaises(ValueError):
                c.validate(source.replace(line, b"", 1), 0)

    def test_original_window_arm_and_measured_span_cannot_be_rebased(self):
        source = report().decode()
        obs = next(line for line in source.splitlines() if "OBS " in line)
        for key in ("first", "last", "arm_target", "arm_observed", "aperture_span", "dispatch_span"):
            value = re.search(r"\b" + key + r"=(\d+):(\d+)", obs)
            replacement = f"{key}={value[1]}:{int(value[2]) + 1}"
            bad = obs.replace(value[0], replacement)
            with self.subTest(key=key), self.assertRaises(ValueError):
                c.validate(source.replace(obs, bad, 1).encode(), 0)
        for key in ("positive", "negative", "attempted", "completed", "frequency", "early_ticks",
                    "residency_ticks", "reads_cap", "vector_removed", "timer_stopped", "signals_restored"):
            # Change the summary only; no clock/epoch repair is made by the observer.
            lines = source.splitlines()
            i = next(i for i, line in enumerate(lines) if "CLOSED " in line)
            lines[i] = re.sub(r"\b" + key + r"=\d+", key + "=0", lines[i])
            with self.subTest(key=key), self.assertRaises(ValueError):
                c.validate(("\n".join(lines) + "\n").encode(), 0)

    def test_failed_rc_cannot_acquire_success_marker_or_hide_an_observation(self):
        for data, code in ((report(), 20), (report(failed=True) + c.PASS.encode() + b"\n", 20),
                           (report() + b"unexpected\n", 0), (report()[:-1], 0),
                           (report().replace(b"index=15", b"index=14"), 0),
                           (report().replace(b"\n", b"\r\n"), 0), (b"X" * 32769, 20)):
            with self.subTest(code=code, size=len(data)), self.assertRaises(ValueError):
                c.validate(data, code)

    def test_schema_matches_existing_native_printf_record(self):
        source = (Path(__file__).resolve().parents[1] / "tests/native_exec_cia_aperture_test.c").read_text()
        literal = re.search(r'printf\("CIA APERTURE OBS (.*?)\\n"', source)[1]
        self.assertEqual(set(re.findall(r"(\w+)=", literal)), c.FIELDS)


if __name__ == "__main__":
    unittest.main()
