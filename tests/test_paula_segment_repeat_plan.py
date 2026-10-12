"""Standalone Paula SEGMENT/REPEAT geometry HOST qualifier.
22 strict units; genuine sampler pins and cache leases; allocation-free planner.
Reader/backend/native/timing qualification remains separate.
"""
from pathlib import Path, PurePosixPath
import hashlib
import json
import os
import posixpath
import re
import stat
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
PRESERVATION_ROOT = ROOT
FIXTURE_PINS = {'src/core/paula_segment_repeat_plan.c': {'bytes': 8911, 'sha256': '06ca2e445a951d14960d4f9cd64efd5041ffb451a7829fcb1cfa7cf3efddd576'}, 'src/core/paula_segment_repeat_plan.h': {'bytes': 6230, 'sha256': '79c93ad567cbe0f811023c7af751a3cd01405108d3b7a6c81ef3f50df3ef218d'}, 'tests/paula_segment_repeat_plan_test.c': {'bytes': 23338, 'sha256': 'a2d4ba2206b27bfdf9d6ee03a7de5ce4ed4c96c57c702b6c4eab57e7efa5e722'}}

def source_path(relative):
    return ROOT / relative
SOURCES = ('tests/paula_segment_repeat_plan_test.c', 'src/core/paula_segment_repeat_plan.c', 'src/core/paula_render_voice.c', 'src/core/voice.c', 'src/editor/sampler_paula.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c')
CFLAGS = ('-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG', '-fsanitize=address,undefined', '-Isrc/core', '-Itests', '-I.')
ORACLE = 'PAULA SEGMENT REPEAT PLAN HOST FIXTURE PASS:52 cases;exact mono8-16-24 initial geometry;even frame-byte-word ranges1-65535;original960-1920-2880-3840;overlap-disjoint loops;genuine voice pending-source handoff;fractional current phase retained;no restart or silent-word inference;full identity and alias refusals;genuine pins and Chip leases retained across stale edits and declarative STOP;OWNERSHIP_READER_NATIVE_INTEGRATION_NOT_RUN\n'
PROTECTED = ('AGENTS.md', 'amiga-test.json', 'src/editor/editor.c', 'src/editor/editor.h', 'src/editor/view.c', 'src/editor/view.h', 'src/native/editor_main.c', 'src/native/pattern_display.c', 'src/native/pattern_display.h', 'tests/editor_test.c', 'tests/native_prepared_test.c', 'tests/test_editor.py', 'tools/build_core_tests.py', 'tools/test_sampler_emulator.py', 'tools/shared_infra_sampler.py', 'tools/shared_infra_ui.py')
RAW_FONT_SHA256 = '56be43ae731975ee8530eda153bd9b1b44329d6af4992b2436d3938d38cb9091'
FONT_HEADER_SHA256 = '0c77c7d479f4afa90bd053e7fc1390ac8397e14faa976aa76297065a645778d0'

def metadata_pin(path):
    actual = path.lstat()
    assert stat.S_ISREG(actual.st_mode), "ordinary regular input required"
    return {name: getattr(actual, name, 0) for name in (
        "st_dev", "st_ino", "st_mode", "st_nlink", "st_size", "st_uid", "st_gid",
        "st_mtime_ns", "st_ctime_ns", "st_flags")}


def ordinary_pin(path):
    before = metadata_pin(path)
    assert not (before["st_flags"] & 0x40000000), "dataless body read refused"
    descriptor = os.open(path, os.O_RDONLY | os.O_NOFOLLOW)
    try:
        opened = os.fstat(descriptor)
        assert {name: getattr(opened, name, 0) for name in before} == before
        body = bytearray()
        while True:
            chunk = os.read(descriptor, 65536)
            if not chunk:
                break
            body.extend(chunk)
        after = os.fstat(descriptor)
        assert {name: getattr(after, name, 0) for name in before} == before
    finally:
        os.close(descriptor)
    assert metadata_pin(path) == before and len(body) == before["st_size"]
    return {"bytes": len(body), "sha256": hashlib.sha256(body).hexdigest(),
            "metadata": before}, bytes(body)


def preservation_snapshot():
    observed = {}
    for relative in PROTECTED:
        path = PRESERVATION_ROOT / relative
        try:
            path.lstat()
        except FileNotFoundError:
            observed[relative] = {"state": "ABSENT"}
        else:
            fact, _ = ordinary_pin(path)
            observed[relative] = {"state": "PRESENT", "pin": fact}
    lock_pin, lock_body = ordinary_pin(PRESERVATION_ROOT / "baseline.lock.json")
    observed["baseline.lock.json"] = lock_pin
    baseline = json.loads(lock_body)
    assert baseline["upstream_commit"] == "4e9df4ada0e55869477e1e45d00227719010e0ea"
    assert len(baseline["files"]) == 51
    for entry in baseline["files"]:
        relative = PurePosixPath(entry["path"])
        assert not relative.is_absolute() and ".." not in relative.parts
        path = PRESERVATION_ROOT / "vendor/pt23f" / relative
        metadata = metadata_pin(path)
        if metadata["st_flags"] & 0x40000000:
            # Unselected placeholders stay metadata-only. No hydration, body
            # hash or current-byte equivalence is inferred from the manifest.
            observed["vendor/pt23f/" + str(relative)] = {
                "state": "DATALESS_METADATA_ONLY", "metadata": metadata,
                "historical_manifest_sha256": entry["sha256"],
                "actual_body_verification": "NOT_PERFORMED"}
        else:
            fact, _ = ordinary_pin(path)
            assert fact["sha256"] == entry["sha256"], "classic baseline changed"
            observed["vendor/pt23f/" + str(relative)] = fact
    return observed


def source_snapshot(private):
    """Copy only the exact units and their literal project include closure."""
    includes = re.compile(rb'^\s*#\s*include\s*"([^"\r\n]+)"', re.MULTILINE)
    pending = list(reversed(SOURCES))
    observed = {}
    while pending:
        relative = pending.pop()
        if relative in observed:
            continue
        path = PurePosixPath(relative)
        assert not path.is_absolute() and ".." not in path.parts
        assert path.parts[0] in ("src", "tests"), "project source only"
        fact, body = ordinary_pin(source_path(relative))
        if relative in FIXTURE_PINS:
            assert fact["bytes"] == FIXTURE_PINS[relative]["bytes"]
            assert fact["sha256"] == FIXTURE_PINS[relative]["sha256"]
        observed[relative] = fact
        copied = private / relative
        copied.parent.mkdir(parents=True, exist_ok=True)
        with copied.open("xb") as stream:
            assert stream.write(body) == len(body)
        copied.chmod(0o400)
        assert ordinary_pin(copied)[0]["sha256"] == fact["sha256"]
        for included in includes.findall(body):
            name = included.decode("ascii")
            if name == "pt_font.h":
                continue  # The independently checked deterministic header.
            assert not PurePosixPath(name).is_absolute()
            selected = None
            for directory in (str(path.parent), "src/core", "tests", "."):
                candidate = posixpath.normpath(posixpath.join(directory, name))
                parts = PurePosixPath(candidate).parts
                if not parts or parts[0] not in ("src", "tests") or ".." in parts:
                    continue
                try:
                    source_path(candidate).lstat()
                except FileNotFoundError:
                    continue
                selected = candidate
                break
            assert selected is not None, "unresolved project include: " + name
            pending.append(selected)
    return observed


def verify_sources(observed):
    for relative, fact in observed.items():
        assert ordinary_pin(source_path(relative))[0] == fact, "source changed: " + relative


class PaulaSegmentRepeatHostPlan(unittest.TestCase):
    def test_geometry_and_genuine_retained_sources(self):
        self.assertIsNotNone(ORACLE, "frozen complete fixture oracle required")
        self.assertEqual(len(SOURCES), 22)
        self.assertEqual(len(set(SOURCES)), 22)
        before = preservation_snapshot()
        source_pins = {}
        try:
            font_pin, raw_font = ordinary_pin(PRESERVATION_ROOT / "vendor/pt23f/raw/ptfont.raw")
            self.assertEqual(font_pin["bytes"], 580)
            self.assertEqual(font_pin["sha256"], RAW_FONT_SHA256)
            font_header = ("static const unsigned char pt_font[580] = {"
                           + ",".join(str(value) for value in raw_font)
                           + "};\n").encode("ascii")
            self.assertEqual(len(font_header), 1630)
            self.assertEqual(hashlib.sha256(font_header).hexdigest(), FONT_HEADER_SHA256)
            with tempfile.TemporaryDirectory(prefix="pt-paula-segment-plan-") as temporary:
                private = Path(temporary)
                source_pins = source_snapshot(private)
                header = private / "pt_font.h"
                header.write_bytes(font_header)
                header.chmod(0o400)
                fixture = private / "fixture"
                environment = {"PATH": "/usr/bin:/bin", "LC_ALL": "C",
                    "TMPDIR": str(private),
                    "ASAN_OPTIONS": "halt_on_error=1:abort_on_error=1",
                    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
                compiled = subprocess.run(
                    ["/usr/bin/cc", *CFLAGS, *SOURCES, "-o", str(fixture)],
                    cwd=private, env=environment,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    check=True, timeout=120)
                self.assertEqual(compiled.stdout, b"")
                self.assertEqual(compiled.stderr, b"")
                actual = subprocess.run(
                    [str(fixture)], cwd=private, env=environment,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    check=True, timeout=30)
                self.assertEqual(actual.stderr, b"")
                self.assertEqual(actual.stdout, ORACLE.encode("ascii"))
                self.assertEqual(ordinary_pin(header)[0]["sha256"], FONT_HEADER_SHA256)
                verify_sources(source_pins)
        finally:
            verify_sources(source_pins)
            self.assertEqual(preservation_snapshot(), before)


if __name__ == "__main__":
    unittest.main()
