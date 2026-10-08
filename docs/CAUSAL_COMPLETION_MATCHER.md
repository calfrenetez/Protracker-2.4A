# Causal completion matcher and pair timer HOST model

The private causal owner now compares a copied first-completion request with its
retained genuine tombstone. The request binds both original identities, serial,
original tick window, actual observed/issued ticks, and masks/all20 post-state
keys. It matches only after real first commit, post-clock validation and reader
adoption while the exact successor is waiting. It remains valid after genuine
first-command disposal, then rejects successor issue or cancellation. Regular
matches and mismatches preserve the complete owner/port/request; existing reentry
handling can latch a fault.

`pt_mixed_causal_first_completion_matches` compares fixed retained metadata by
value under the same whole-entry exclusion contract as fire. Pointer fields stay
opaque; it does not traverse events, queues, spans, domains or arbitrary output
ranges, call a clock/backend, or service a reader. The ordinary-RAM dispatch uses
its actual entry/pre/post clock phases and the matcher instead of the task-only
copied diagnostic. Actual raw adoption remains distinct from a later check
failure so possibly held references cannot be retired as unused.

`pt_mixed_causal_diagnostic` remains a **task-only diagnostic**, never completion
authority, readiness or quiet evidence. Removing its scan from dispatch removes a
known software prerequisite; it does not establish native IRQ eligibility.

The separate pair timer source is a **HOST resource model**. Its installed
source-specific entry selects the due copied ticket from resource state; callers
supply no ticket. It preserves original960/1920 deadlines, EARLY absolute rearm,
whole task/delivery exclusion and prior state, independent first command/readers
and successor, uncertain/reentrant effects, once-only removal and subsequent
read-only quiet. Compilation requires the explicit host-model define and refuses
Amiga/m68k targets. It is not an actual CIA/Exec provider, and the old single-armed
source cannot be cast to this interface.

## Accepted evidence

| Distinct operation | Genuine cases/checks | Full stdout | Compile/run/outer seconds |
| --- | --- | --- | --- |
| Matcher V5,80 units with ASan/UBSan | Existing34 plus2 added lifecycles,219 matcher checks | 842 bytes | 19.392 /0.675 /22.220 |
| Pair timer V2,81 units with ASan/UBSan and host define | 10 owner/model cases | 352 bytes | 19.522 /0.461 /22.726 |

Both operations passed complete literal stdout, empty compiler streams/runtime
stderr, strict before/after input custody, public RC0/reap/group absence and exact
owned source cleanup under the separate120/30/240-second bounds. The timer group
did not replay baseline34 or matcher219. Publishing them together is not a joint
runtime qualification.

The durable [matcher evidence](../evidence/enhanced-editor/causal-completion-matcher-host/README.md)
and [timer evidence](../evidence/enhanced-editor/causal-pair-timer-host-model/README.md)
contain the full streams, inner/outer receipts, saved audits and copy manifests.
Their complete runtime oracles are available as
[matcher stdout](../evidence/enhanced-editor/causal-completion-matcher-host/raw/run.stdout)
and [timer stdout](../evidence/enhanced-editor/causal-pair-timer-host-model/raw/run.stdout).
Matcher V1/V2/V3/V4 failures remain preserved and are not retroactively admitted.
The successful private workspace does not identify the earlier drift's cause.

Native entry ABI, stack/full-entry WCET, interrupt exclusion/residency and actual
timer delivery/restoration/cessation still need separate qualification, as do
Amiberry, real A1200, DMA/voice stop, sample-RAM capacity/upload/order/completion,
audio and listening for these changes. The user's
[exact scheduling decision](NATIVE_TIMING_DECISION.md) remains unchanged. Default
PLAY, the baseline34 fixture, existing runner,16 unrelated protected paths and
accepted layout are preserved.

## Literal HOST reproduction recipes

Run from the repository root in Bash. These expose the accepted source lists,
flags and full byte oracles without invoking donor tests or altering the existing
runner. The shell recipes have not been invoked as a combined script; acceptance
belongs to the guarded captured operations above. Reproduction alone does not
supply their whole-operation custody/cleanup admission.

Create a private source/font copy and carry the durable literal oracles into it:

```bash
set -e
PT_PAIR_HOST_DIR="$(mktemp -d /private/tmp/pt24g-causal-pair-repro.XXXXXX)"
cp -R src tests "$PT_PAIR_HOST_DIR/"
cp evidence/enhanced-editor/causal-completion-matcher-host/raw/run.stdout "$PT_PAIR_HOST_DIR/matcher.expected.stdout"
cp evidence/enhanced-editor/causal-pair-timer-host-model/raw/run.stdout "$PT_PAIR_HOST_DIR/timer.expected.stdout"
python3 - "$PT_PAIR_HOST_DIR/pt_font.h" <<'PYFONT'
from pathlib import Path
import hashlib, sys
raw = Path('vendor/pt23f/raw/ptfont.raw').read_bytes()
assert len(raw) == 580
assert hashlib.sha256(raw).hexdigest() == '56be43ae731975ee8530eda153bd9b1b44329d6af4992b2436d3938d38cb9091'
header = ('static const unsigned char pt_font[580] = {' + ','.join(str(b) for b in raw) + '};\n').encode('ascii')
assert hashlib.sha256(header).hexdigest() == '0c77c7d479f4afa90bd053e7fc1390ac8397e14faa976aa76297065a645778d0'
Path(sys.argv[1]).write_bytes(header)
PYFONT
cd "$PT_PAIR_HOST_DIR"
PT_PAIR_FLAGS=(-std=c99 -O1 -g -Wall -Wextra -Werror -UNDEBUG -fsanitize=address,undefined -Isrc/core -I.)
export ASAN_OPTIONS=halt_on_error=1:abort_on_error=1
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
PT_PAIR_COMMON=(
  src/native/readers_ram/native_mixed_causal_ram_port.c src/editor/editor_mixed_causal_prepare.c src/editor/sampler_mixed_readers.c
  src/core/mixed_readers_causal.c src/core/amigus_trigger_levels.c src/core/mixed_scheduled_readers.c
  src/core/elapsed_clock.c src/editor/sampler_paula.c src/editor/sampler_wavetable.c
  src/core/amigus_voice_plan.c src/editor/sampler.c src/editor/slots.c
  src/core/pcm_filtered.c src/core/slices.c src/core/pattern.c
  src/core/document.c src/core/pp20.c src/core/project.c
  src/core/mod_project.c src/core/mod_inspect.c src/core/channels.c
  src/core/pcm.c src/core/wav.c src/core/svx.c
  src/core/raw.c src/editor/editor.c src/editor/song.c
  src/editor/view.c src/editor/workflow.c src/editor/sample_range.c
  src/editor/wave_summary.c src/core/sample_usage.c src/core/event_resource.c
  src/core/flow.c src/core/pitch.c src/core/amigus_reservation.c
  src/editor/sampler_invert_song.c src/core/render_invert.c src/core/invert_bank.c
  src/core/invert_sequence.c src/core/invert_pcm.c src/core/invert_loop.c
  src/core/amigus_session.c src/core/amigus_fifo.c src/core/amigus_pcm_pack.c
  src/core/studio_consumer.c src/core/studio_pump.c src/core/studio_queue.c
  src/editor/editor_studio.c src/editor/sampler_song.c src/editor/sampler_studio.c
  src/core/studio_song.c src/core/studio_plan.c src/core/render.c
  src/core/timeline.c src/core/frame_clock.c src/core/studio_mix.c
  src/core/voice.c src/editor/wavetable_song.c src/editor/wavetable_voices.c
  src/editor/wavetable_dispatch.c src/core/amigus_render_voice.c src/editor/paula_preflight.c
  src/core/paula_render_voice.c src/core/amigus_wavetable_cache.c src/core/amigus_sample_ram.c
  src/core/sample_cache.c src/core/playback_pcm.c src/editor/editor_mixed.c
  src/editor/mixed_owner.c src/editor/mixed_transport.c src/editor/mixed_preflight.c
  src/editor/paula_voices.c src/editor/paula_dispatch.c src/editor/editor_wavetable.c
  src/platform/sample_import.c src/platform/raw_import.c src/platform/mod_import.c
  src/platform/pp20_import.c
)
```

Matcher: the new fixture's own main hook runs the unchanged34-case baseline first,
then its2 added genuine lifecycles and all219 checks. The80-unit compile and full
842-byte output are separate from the timer group:

```bash
/usr/bin/cc "${PT_PAIR_FLAGS[@]}" tests/native_mixed_causal_completion_matcher_test.c "${PT_PAIR_COMMON[@]}" -o matcher-host >matcher.compile.stdout 2>matcher.compile.stderr
test ! -s matcher.compile.stdout && test ! -s matcher.compile.stderr
./matcher-host >matcher.run.stdout 2>matcher.run.stderr
test ! -s matcher.run.stderr
cmp matcher.run.stdout matcher.expected.stdout
shasum -a 256 matcher.run.stdout
```

Expected SHA256: `d07f72df7e36ba1cd973efe43b9266ec5dacc0a0bc270e5968b480e32403bcb1`.

Timer: one new production C unit makes81; the define must be supplied to that
separate translation unit. The timer fixture's local define alone is insufficient.
Only its complete352-byte model oracle is expected:

```bash
/usr/bin/cc "${PT_PAIR_FLAGS[@]}" -DPT_PRIVATE_PAIR_TIMER_HOST_RESOURCE_MODEL=1 tests/native_mixed_causal_pair_timer_source_test.c src/native/readers_ram/native_mixed_causal_pair_timer_source.c "${PT_PAIR_COMMON[@]}" -o pair-timer-host >timer.compile.stdout 2>timer.compile.stderr
test ! -s timer.compile.stdout && test ! -s timer.compile.stderr
./pair-timer-host >timer.run.stdout 2>timer.run.stderr
test ! -s timer.run.stderr
cmp timer.run.stdout timer.expected.stdout
shasum -a 256 timer.run.stdout
```

Expected SHA256: `6987cebd788b7460319820cb632ca639ea1dce7da9cc7190ce99579b2002b81c`.
The private source tree and captures remain at `$PT_PAIR_HOST_DIR` for review.
