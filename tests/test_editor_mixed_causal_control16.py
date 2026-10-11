"""Focused CONTROL16 HOST unittest; native scheduling and device stop are separate."""
from pathlib import Path
import hashlib
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = (
    'tests/editor_mixed_causal_control16_prepare_test.c',
    'src/editor/editor_mixed_causal_prepare.c',
    'src/editor/sampler_mixed_readers.c',
    'src/core/mixed_readers_causal.c',
    'src/core/amigus_trigger_levels.c',
    'src/core/mixed_scheduled_readers.c',
    'src/core/elapsed_clock.c',
    'src/editor/sampler_paula.c',
    'src/editor/sampler_wavetable.c',
    'src/core/amigus_voice_plan.c',
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
    'src/editor/editor.c',
    'src/editor/song.c',
    'src/editor/view.c',
    'src/editor/workflow.c',
    'src/editor/sample_range.c',
    'src/editor/wave_summary.c',
    'src/core/sample_usage.c',
    'src/core/event_resource.c',
    'src/core/flow.c',
    'src/core/pitch.c',
    'src/core/amigus_reservation.c',
    'src/editor/sampler_invert_song.c',
    'src/core/render_invert.c',
    'src/core/invert_bank.c',
    'src/core/invert_sequence.c',
    'src/core/invert_pcm.c',
    'src/core/invert_loop.c',
    'src/core/amigus_session.c',
    'src/core/amigus_fifo.c',
    'src/core/amigus_pcm_pack.c',
    'src/core/studio_consumer.c',
    'src/core/studio_pump.c',
    'src/core/studio_queue.c',
    'src/editor/editor_studio.c',
    'src/editor/sampler_song.c',
    'src/editor/sampler_studio.c',
    'src/core/studio_song.c',
    'src/core/studio_plan.c',
    'src/core/render.c',
    'src/core/timeline.c',
    'src/core/frame_clock.c',
    'src/core/studio_mix.c',
    'src/core/voice.c',
    'src/editor/wavetable_song.c',
    'src/editor/wavetable_voices.c',
    'src/editor/wavetable_dispatch.c',
    'src/core/amigus_render_voice.c',
    'src/editor/paula_preflight.c',
    'src/core/paula_render_voice.c',
    'src/core/amigus_wavetable_cache.c',
    'src/core/amigus_sample_ram.c',
    'src/core/sample_cache.c',
    'src/core/playback_pcm.c',
    'src/editor/editor_mixed.c',
    'src/editor/mixed_owner.c',
    'src/editor/mixed_transport.c',
    'src/editor/mixed_preflight.c',
    'src/editor/paula_voices.c',
    'src/editor/paula_dispatch.c',
    'src/editor/editor_wavetable.c',
    'src/platform/sample_import.c',
    'src/platform/raw_import.c',
    'src/platform/mod_import.c',
    'src/platform/pp20_import.c',
)
ORACLE = 'EDITOR MIXED CAUSAL CONTROL16 PREPARE PASS:25 genuine instances;6 mixed/card8/16/24master8/16cache schedules exact0/255/256/65535 copied levels;2 unchanged uint8 schedules;6 full-input/workspace/reader/master output aliases;3 actual-first/stale/duplicate authority refusals;5 consumed allocator/reentry/input/transformed-scratch outcomes;1 lower enqueue OK amid outer fault;2 direct core whole-input/holder aliases;original960/1920/2880 windows,genuine C1 service/NULL close,C3 old STOP,zero later readers/pins/cache/upload,independent quiet,complete master/save custody;SOFTWARE_ONLY\n'
RAW_FONT_SHA256 = "56be43ae731975ee8530eda153bd9b1b44329d6af4992b2436d3938d38cb9091"
FONT_HEADER_SHA256 = "0c77c7d479f4afa90bd053e7fc1390ac8397e14faa976aa76297065a645778d0"


class EditorMixedCausalControl16Host(unittest.TestCase):
    def test_exact_wide_control_and_original_reader_lifetimes(self):
        raw_font = (ROOT / "vendor/pt23f/raw/ptfont.raw").read_bytes()
        self.assertEqual(len(raw_font), 580)
        self.assertEqual(hashlib.sha256(raw_font).hexdigest(), RAW_FONT_SHA256)
        font_header = ("static const unsigned char pt_font[580] = {"
                       + ",".join(str(value) for value in raw_font)
                       + "};\n").encode("ascii")
        self.assertEqual(len(font_header), 1630)
        self.assertEqual(hashlib.sha256(font_header).hexdigest(), FONT_HEADER_SHA256)
        with tempfile.TemporaryDirectory(prefix="pt-editor-causal-control16-") as temporary:
            private = Path(temporary)
            header = private / "pt_font.h"
            header.write_bytes(font_header)
            header.chmod(0o400)
            fixture = private / "fixture"
            env = dict(os.environ, TMPDIR=str(private),
                       ASAN_OPTIONS="halt_on_error=1:abort_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
            built = subprocess.run(
                ["/usr/bin/cc", "-std=c99", "-O1", "-g", "-Wall", "-Wextra",
                 "-Werror", "-UNDEBUG", "-fsanitize=address,undefined",
                 "-I" + str(private), "-I.", "-Isrc/core", *SOURCES,
                 "-o", str(fixture)],
                cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                check=True, timeout=120)
            self.assertEqual(built.stdout, b"")
            self.assertEqual(built.stderr, b"")
            actual = subprocess.run(
                [str(fixture)], cwd=ROOT, env=env,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                check=True, timeout=30)
            self.assertEqual(actual.stderr, b"")
            self.assertEqual(actual.stdout, ORACLE.encode("ascii"))


if __name__ == "__main__":
    unittest.main()
