"""HOST sanitizer qualification of the exact native rolling68 entry.

The original assertions/oracle are retained; only the exact complete terminal
success string maps to a short native capture marker. All clocks/buses are
software models, and no native dispatch or hardware output is enabled.
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
SOURCES = ('tests/native_exec_mixed_causal_rolling_test.c', 'src/editor/editor_mixed_causal_prepare.c', 'src/editor/sampler_mixed_readers.c', 'src/core/mixed_readers_causal.c', 'src/core/amigus_trigger_levels.c', 'src/core/mixed_scheduled_readers.c', 'src/core/elapsed_clock.c', 'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/editor/editor.c', 'src/editor/song.c', 'src/editor/view.c', 'src/editor/workflow.c', 'src/editor/sample_range.c', 'src/editor/wave_summary.c', 'src/core/sample_usage.c', 'src/core/event_resource.c', 'src/core/flow.c', 'src/core/pitch.c', 'src/core/amigus_reservation.c', 'src/editor/sampler_invert_song.c', 'src/core/render_invert.c', 'src/core/invert_bank.c', 'src/core/invert_sequence.c', 'src/core/invert_pcm.c', 'src/core/invert_loop.c', 'src/core/amigus_session.c', 'src/core/amigus_fifo.c', 'src/core/amigus_pcm_pack.c', 'src/core/studio_consumer.c', 'src/core/studio_pump.c', 'src/core/studio_queue.c', 'src/editor/editor_studio.c', 'src/editor/sampler_song.c', 'src/editor/sampler_studio.c', 'src/core/studio_song.c', 'src/core/studio_plan.c', 'src/core/render.c', 'src/core/timeline.c', 'src/core/frame_clock.c', 'src/core/studio_mix.c', 'src/core/voice.c', 'src/editor/wavetable_song.c', 'src/editor/wavetable_voices.c', 'src/editor/wavetable_dispatch.c', 'src/core/amigus_render_voice.c', 'src/editor/paula_preflight.c', 'src/core/paula_render_voice.c', 'src/core/amigus_wavetable_cache.c', 'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c', 'src/editor/editor_mixed.c', 'src/editor/mixed_owner.c', 'src/editor/mixed_transport.c', 'src/editor/mixed_preflight.c', 'src/editor/paula_voices.c', 'src/editor/paula_dispatch.c', 'src/editor/editor_wavetable.c', 'src/platform/sample_import.c', 'src/platform/raw_import.c', 'src/platform/mod_import.c', 'src/platform/pp20_import.c', 'src/editor/editor_mixed_causal_song_pair.c', 'src/editor/editor_mixed_causal_song_lineage.c', 'src/editor/sampler_establish.c', 'src/editor/mixed_quantized_audit.c', 'src/editor/mixed_readers_plan_normalize.c', 'src/native/readers_ram/native_mixed_causal_ram_port.c', 'src/editor/sampler_lifetime.c')
CFLAGS = ('-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG', '-fsanitize=address,undefined', '-Isrc/core', '-Itests', '-I.')
ORACLE = 'ROLLING RAM NATIVE PASS:68 cases;T16-C16-C16-S16;actual row retirement and original reader pins;960-1920-2880-3840;MODELED CLOCK/BUS ONLY\n'
PROTECTED = ('AGENTS.md', 'amiga-test.json', 'src/editor/editor.c', 'src/editor/editor.h', 'src/editor/view.c', 'src/editor/view.h', 'src/native/editor_main.c', 'src/native/pattern_display.c', 'src/native/pattern_display.h', 'tests/editor_test.c', 'tests/native_prepared_test.c', 'tests/test_editor.py', 'tools/build_core_tests.py', 'tools/test_sampler_emulator.py', 'tools/shared_infra_sampler.py', 'tools/shared_infra_ui.py')
RAW_FONT_SHA256 = '56be43ae731975ee8530eda153bd9b1b44329d6af4992b2436d3938d38cb9091'
FONT_HEADER_SHA256 = '0c77c7d479f4afa90bd053e7fc1390ac8397e14faa976aa76297065a645778d0'


def stable_fact(value):
    """Ignore only cloud ctime/residency bookkeeping across separate reads.

    SHA256, length, identity, mode, uid/gid, links and mtime still compare.
    Within each ordinary read all raw metadata checks remain strict.
    """
    if isinstance(value, dict):
        return {key: stable_fact(item) for key,item in value.items()
                if key not in ('st_ctime_ns','st_flags')}
    if isinstance(value, list):
        return [stable_fact(item) for item in value]
    return value


def evidence(name, value):
    selected = os.environ.get('PT_ROLLING_ENTRY_HOST_EVIDENCE')
    if not selected:
        return
    directory = Path(selected)
    assert directory.is_absolute() and directory.is_dir() and not directory.is_symlink()
    path = directory / name
    with path.open('xb') as stream:
        stream.write(value if isinstance(value, bytes) else (json.dumps(value,indent=2)+'\n').encode('utf-8'))


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
        path = ROOT / relative
        try:
            path.lstat()
        except FileNotFoundError:
            observed[relative] = {"state": "ABSENT"}
        else:
            fact, _ = ordinary_pin(path)
            observed[relative] = {"state": "PRESENT", "pin": fact}
    lock_pin, lock_body = ordinary_pin(ROOT / "baseline.lock.json")
    observed["baseline.lock.json"] = lock_pin
    baseline = json.loads(lock_body)
    assert baseline["upstream_commit"] == "4e9df4ada0e55869477e1e45d00227719010e0ea"
    assert len(baseline["files"]) == 51
    for entry in baseline["files"]:
        relative = PurePosixPath(entry["path"])
        assert not relative.is_absolute() and ".." not in relative.parts
        path = ROOT / "vendor/pt23f" / relative
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
        fact, body = ordinary_pin(ROOT / relative)
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
                    (ROOT / candidate).lstat()
                except FileNotFoundError:
                    continue
                selected = candidate
                break
            assert selected is not None, "unresolved project include: " + name
            pending.append(selected)
    return observed


def verify_sources(observed):
    for relative, fact in observed.items():
        assert stable_fact(ordinary_pin(ROOT / relative)[0]) == stable_fact(fact), "source changed: " + relative


class NativeMixedCausalRollingNativeEntryHost(unittest.TestCase):
    def test_exact_native_68_case_entry_under_sanitizers(self):
        self.assertIsNotNone(ORACLE, "frozen complete fixture oracle required")
        self.assertEqual(len(SOURCES), 86)
        self.assertEqual(len(set(SOURCES)), 86)
        before = preservation_snapshot()
        evidence("before-preservation.json", before)
        source_pins = {}
        try:
            font_pin, raw_font = ordinary_pin(ROOT / "vendor/pt23f/raw/ptfont.raw")
            self.assertEqual(font_pin["bytes"], 580)
            self.assertEqual(font_pin["sha256"], RAW_FONT_SHA256)
            font_header = ("static const unsigned char pt_font[580] = {"
                           + ",".join(str(value) for value in raw_font)
                           + "};\n").encode("ascii")
            self.assertEqual(len(font_header), 1630)
            self.assertEqual(hashlib.sha256(font_header).hexdigest(), FONT_HEADER_SHA256)
            with tempfile.TemporaryDirectory(prefix="pt-causal-ram-rolling-") as temporary:
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
                evidence("compile.stdout", compiled.stdout)
                evidence("compile.stderr", compiled.stderr)
                self.assertEqual(compiled.stdout, b"")
                self.assertEqual(compiled.stderr, b"")
                actual = subprocess.run(
                    [str(fixture)], cwd=private, env=environment,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    check=True, timeout=30)
                evidence("fixture.stdout", actual.stdout)
                evidence("fixture.stderr", actual.stderr)
                self.assertEqual(actual.stderr, b"")
                self.assertEqual(actual.stdout, ORACLE.encode("ascii"))
                self.assertEqual(ordinary_pin(header)[0]["sha256"], FONT_HEADER_SHA256)
                verify_sources(source_pins)
        finally:
            verify_sources(source_pins)
            after = preservation_snapshot()
            evidence("after-preservation.json", after)
            self.assertEqual(stable_fact(after), stable_fact(before))


if __name__ == "__main__":
    unittest.main()
