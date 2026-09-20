# Shared sequencer and offline replay: implementation boundary

The row/tick flow core and native trace comparison are implemented in dev28.
Dev29 adds the explicit ideal-BPM reference frame clock and transactional timeline.
The remaining sections describe the next implementation boundary, not an
implemented renderer or an audio acceptance claim. It uses the pinned local 2.3F CIA replay source at
`vendor/pt23f/replayer/PT2.3F_replay_cia.s` (upstream commit recorded in
`baseline.lock.json`) as the classic compatibility reference. The existing native
Paula wrapper and MIDI ownership/recording cores remain separate working pieces.

## First deliverable: deterministic tick and row flow

A caller-owned, allocation-free sequencer should read a validated immutable
project snapshot and emit ordered per-track commands to an injected sink. All
routes share one clock. Physical Paula/AmiGUS/CAMD adapters and the offline mixer
consume that same sequence; they must not each interpret global effects or keep
independent song positions. Snapshot replacement is an explicit stop/restart
boundary until a measured safe live-publication design exists.

Implement and test row/tick traversal before mixing PCM. Trace records must
identify the row being processed separately from the already-advanced next-row
cursor. Preserve raw periods and MIDI note numbers in their distinct domains.
Extend channel traversal to 16 in ascending track order, preserving the pinned
four-channel order for classic comparisons. Stop/limit/cancel are explicit
outcomes; an exhausted render budget is not a successfully completed song.

The source requires ordered state transitions, not a generic last-global-command
rule. In particular:

- `mt_PositionJump` sets the position and clears the break row. A later Dxx can
  then supply a break row; reversing the channel order can change the result.
- Dxx interprets the high nibble as tens and the low nibble as units, then applies
  its row-range check. Do not substitute an unrelated hexadecimal row policy.
- F01..F1F changes speed and clears the counter; F20..FF changes tempo; F00 follows
  the stop path. Multiple Fxx events need pinned native traces before asserting
  a precedence policy, especially when stop and later commands coexist.
- E6x retains loop-start/count state per track but changes shared break state.
  Interactions with Bxx/Dxx and the natural row-63 boundary need dedicated traces.
- EEx uses two delay counters. Delayed passes run effect processing without
  fetching a fresh note row; a simple repeated-row implementation can retrigger
  notes or reset delay state incorrectly.

Use integer time accumulation with an explicit clock profile. Retain fractional
output-frame remainder across tick boundaries and tempo changes. Do not claim
PAL CIA timer parity from the idealized `2.5 / BPM` duration alone: compare the
pinned timer quantization and first-row startup behavior independently.

Initial fixtures must cover speed 1/6/31, tempo boundaries, zero/F00, row 63,
natural wrap, forward/backward jumps, opposite channel orders of B/D, invalid
Dxx destinations, nested/interacting track loops, delay plus jump/break, repeated
positions, one-pattern mode, and bounded nontermination. Compare host traces and
native trace fixtures before wiring a duration display or rendering UI.

The input corpus is prepared by `tools/make_replay_flow_fixtures.py NEW_DIRECTORY`.
It generates sixteen reproducible, synthetic two-pattern MODs plus exact hashes,
event locations and tick budgets. Host preflight checks every fixture with the
shared strict MOD parser. This generator produces **input fixtures only**, so its manifest always says
NOT RUN. Separately captured native traces and portable control-flow comparisons
are retained under `evidence/enhanced-editor/dev28/`. The generator refuses an
existing output directory so it cannot replace prior evidence.

## Subsequent PCM and export boundary

Build sample/loop/slice triggering and supported classic effects on those traces.
Keep 24-bit source precision through signed wide accumulation; clipping and final
16/24-bit quantization are explicit output operations. Test stereo channel order,
pan endpoints, mute/solo/groups, forward/pingpong loops, slices and rate changes
with small hand-verifiable fixtures before longer output hashes. Mutable effects
must operate on owned playback storage, never the editable project's PCM.

Offline WAV/stems/bounce must refuse an unsupported effect or route before
publishing a completed result. External MIDI sound is unavailable unless supplied
as captured audio; a MIDI-only track cannot silently become an audio stem. Bound
time, output size and temporary allocations, allow cancellation, validate staged
output, and reuse the new-file save boundary. Bounce must enter a selected sample
slot through the shared journal as one atomic command after rendering succeeds.

Hardware sound, low-end timing, Studio capacity and underrun acceptance remain
physical tests. An offline reference implementation can provide deterministic
software evidence without claiming those results.

## Native trace capture plan

Use a separately linked diagnostic replay object, not instrumentation in the
shipping editor. The existing ABI already resets persistent voice/loop/jump state
on every start and records the played order/row at `mt_GetNewNote`. A diagnostic
can add a bounded end-of-tick record immediately before the ISR restores its
saved registers. Disabled pre-start interrupts must bypass recording; a tick
that executes F00 must still publish its final record.

A fixed record should capture tick number, played/next order and row, tick
counter, speed, real tempo, enable state, new/active delay counters, break/jump
flags, each track's loop start/count, fresh-row fetch count and final raw volumes.
Publish the record count last. Allocate the buffer before acquiring the replay
resources, never allocate or print in the interrupt, and stop/release the engine
before reading/freeing the trace. Keep deadlines and trace-budget exhaustion
separate from an observed F00 stop.

Run each synthetic fixture twice from fresh engine state and compare complete
records, excluding wall-clock time and addresses. Preserve the trace bytes,
fixture/source/compiler/binary identities and stop reason. These captured traces
then become the oracle for the portable flow core. Capturing a repeatable trace
alone is not a claim that a new sequencer matches it, or that all audio effects
have been implemented.

## Implemented control-flow core (dev28)

`src/core/flow.c` is an allocation-free state machine over a validated immutable
project. It handles startup ticks, speed/tempo state, Bxx/Dxx order and break
interaction, per-track E6 loops, EEx delayed passes and F00 stops. It identifies
fresh row fetches separately from delayed tick-zero passes and retains both the
played cursor and the next cursor. This is only control flow: ignoring voice
commands here does not mean they can be ignored by an audio renderer.

Classic mode explicitly requires four tracks, at most 128 positions and initial
6/125 timing. Extended mode accepts the project's 1..16 tracks, up to 256 positions
and initial timing; it traverses all tracks in ascending order and wraps order
indices at 256. Global effects still run on muted/MIDI tracks. Bxx continues to
address its literal byte-valued position. Callers supply a nonzero tick budget;
STOPPED and LIMIT are distinct, stable outcomes. The native F00 tick is emitted
before STOPPED, even if later tracks on that row change speed. The first row is
fetched after the initial speed-count lead-in, matching the pinned native ABI.

Sixteen native synthetic cases were captured twice with exactly matching 36-byte
records. All first 30 bytes (flow state) match the portable core on both the
host and emulated 68030 (766 ticks). The
remaining six bytes hold native DMA/raw-volume observations and are not treated
as portable audio parity. Tests also exercise extended channel/order boundaries
and failure-atomic initialization. Native trace clocks record tick counts and BPM,
not measured elapsed CIA cycles; timer latch latency, fractional output frames
and PCM effects remain subsequent work. No duration estimate or rendering UI is
wired to this core yet.

The oracle includes source-specific details worth preserving: B01 then D12 enters
order 1, row 12; reversing them enters row 0. D0A enters row 10. F00 then F06 still
stops after processing that row. EE2 with D03 preserves delayed processing and
the next fresh fetch is order 1, row 4. E6 loop start state persists across orders.
These are observed reference results, not assumptions from a generic MOD player.

The diagnostic is built as `PTFlowTraceTest` with a separate `replay_trace.o`.
`PT24GEdit` and `PTPaulaTest` keep the uninstrumented replay object. Every record
is published at ISR exit into preallocated memory, disabled setup interrupts
bypass the recorder, and reaching capacity disables further diagnostic ticks.
A simultaneous F00 retains the native-stop reason. No printing/allocation occurs
in the ISR; the task removes the interrupt and stops DMA before reading/freeing.

Trace format schema 1, big endian, 36 bytes per completed tick:

| Offset | Bytes | Field |
| --- | --- | --- |
| 0 | 4 | Tick number, starting at 1 |
| 4, 5 | 1 each | Played order, next order |
| 6, 8 | 2 each | Played row bytes, next row bytes (divide by 16) |
| 10, 11 | 1 each | Tick counter, speed |
| 12 | 2 | Real BPM |
| 14..19 | 1 each | Enabled, pending delay, active delay, break row, jump flag, loop-break flag |
| 20 | 4 | Per-track loop start rows |
| 24 | 4 | Per-track loop counts |
| 28 | 2 | Fresh row fetch count |
| 30 | 2 | Native DMA command mask |
| 32 | 4 | Native raw output volumes |

Native booleans are 0/255; portable booleans are 0/1. The comparison explicitly
normalizes them. Poll scheduling, wall-clock times and pointers are excluded from
the oracle. Exact repeats prove deterministic state capture for this corpus,
not all possible effects, real-hardware performance or cycle-accurate audio.


## Ideal reference frame clock and timeline (dev29)

`frame_clock.c` implements an explicitly idealized `2.5 / BPM` tick interval at a
chosen integer output rate. It carries unsigned Q32 fractional frames across all
ticks and tempo changes; each interval truncates less than 2^-32 of one frame.
Consequently the accumulated fixed-point value is below the exact rational total
by less than `tick_count / 2^32` frames. Integer frame totals are the floor of that
fixed-point accumulator. At an exact rational integer boundary they may be one
frame below an exact-rational implementation; this is a specified deterministic
reference policy, not a claim of mathematically exact rational accumulation.

Rates 1..192000 and tempos 32..255 are accepted. Zero-frame intervals are valid at
low rates. The caller supplies a nonzero total-frame budget; invalid input, alias,
limit and overflow refusals leave both clock and output unchanged. No floating
point, allocation, dependency on host word size or wall time is used.

`timeline.c` binds this clock to the flow core as one transaction. A successful
step returns the frame span **before** the completed tick. A renderer must produce
that span with its previous voice state, then apply the tick's commands at the
span's end. The old BPM times the elapsed interval, and a newly encountered Fxx
applies to the next interval. This preserves the initial speed-tick lead-in and
locates the final F00 at an explicit output boundary. A frame-limit refusal does
not consume a row or alter loop/tempo/clock state. Stop, tick-limit and frame-limit
outcomes are distinct and do not replace the previous returned span.

This profile is named `IDEAL_BPM_Q32` in retained evidence. It intentionally has no
PAL/NTSC/CIA selector: modeling native timer-latch quantization, reload latency
and measured clocks still requires separate evidence. A future renderer must
record this profile in its output/provenance; it must not present it as captured
Paula/AmiGUS audio. The next stage is voice/sample mixing on the shared timeline.


## Immutable reference voice and mixer (dev30)

`voice.c` is an allocation-free PCM voice primitive, independent of tracker-effect
interpretation. It borrows validated immutable mono/stereo 8/16/24-bit PCM and an
explicit half-open playback range. A positive Q32 step specifies source frames
per output frame. The caller is responsible for deriving that step from pitch,
sample rate, finetune and the chosen clock policy; those tracker semantics are
not supplied by this primitive.

One-shot playback becomes silent at the range end. Forward loops wrap to their
start. Ping-pong loops reflect continuously between their first and last frames,
without duplicating endpoints; a one-frame loop remains constant. Loops must be
wholly contained in the selected range. The voice plays any prefix before the
loop and then stays in the loop. Whole/fractional steps, including steps spanning
many loops, use bounded modular arithmetic. Ping-pong loop size is limited to
2^31 frames to keep the doubled Q32 cycle representable. The normal document and
memory policies are far smaller.

Nearest-frame and optional linear interpolation are explicit policies. Linear
interpolation follows forward-loop seams and mirrored ping-pong positions,
clamping the final one-shot endpoint. It rounds to signed24, ties away from zero.
Mono is duplicated into two sides; stereo order is retained. Eight- and sixteen-bit
inputs are scaled exactly into the signed24 domain. Linear interpolation here
is not the filtered offline resampler and does not provide antialias filtering.

The block mixer supports up to sixteen initialized voices (zeroed unused slots
are silent). Independent Q16 left/right gains are supplied by its caller. It does
not guess a pan law, routing, mute/solo or group policy; setting a gain to zero
still advances that voice. Products accumulate in signed64, with one final
round/quantize/saturate step into stereo16 or stereo24. Output clipping counts
clipped sample values. There is no automatic normalization or dither. PCM remains
untouched, and failed capacity/gain/alias preflight preserves voices and outputs.

Tests use hand-calculated traversal/interpolation/format/clipping cases and an
independent Python unbounded-integer trajectory oracle. A UINT64_MAX phase step,
large forward and ping-pong skips, asymmetric stereo, 16-voice accumulation,
block partitioning and mixed inactive/muted voices are covered. This primitive
provides software sample traversal/mixing evidence, not complete ProTracker
period/effect parity or hardware AmiGUS sound. Sample slices can use explicit
range bounds, but a renderer still has to resolve project slice ordinals and
apply the agreed loop/trigger policy before calling it.


## Streaming reference integration (dev31)

The first integration is now `render.c`, `platform/render_file.c` and the native
`PT24GRender` utility. See [its contract](REFERENCE_RENDERER.md) for the exact
supported subset and profiles. The flow core additionally counts order transitions
and returns to the same/lower order. E6 row loops do not increment these counters.
The renderer preserves the outgoing row interval and ends at the next fresh row
boundary after a position return, or at F00. File output is bounded, streaming,
re-rendered for byte comparison and atomically published with no replacement.
This does not turn the remaining voice/effect and physical gates into passes.


## Pitch-register reference extension (dev35)

`pitch.c` consumes the completed shared flow tick for selected tracks. It keeps
stored words and output writes separate; deriving every output from a clamped
base period loses the original replay's wrap/PerNop/SetBack behavior. Fresh rows
also retain the original previous-empty-event check. Raw project-note periods
remain the reference input policy; this is not native note-table quantization.

`prepare_pitch_trace.py` creates a separate 52-byte diagnostic record. The first
36 bytes preserve the flow diagnostic schema; offsets36..43 contain four stored
period words and offsets44..51 the last four hardware period writes. All words
are big-endian. Ten period-write sites are guarded by an exact anchor count.
Wrappers preserve registers and MOVE.W flags, including X, then perform the
original hardware write. ISR buffers are allocated before playback; publication
of the record count still occurs last. The diagnostic never enters PT24GEdit.
The baseline fixture's original 36 fields remain identical to retained dev28
records. No previous traces are rewritten.

The renderer runs pitch interpretation in measurement as well as streaming.
A triggered zero output period fails measurement with PT_RENDER_EFFECT before
any file staging, sink calls or sample append. Remaining classic effects and
zero-period sound semantics require further evidence; no hardware sound model
is inferred from these register traces.


## Tone-portamento state (dev36)

3xx remembers its speed on an effect pass, including a delayed pass; a parameter
on a fresh speed-one row can therefore go unused. 5xx invokes the remembered
glide independently of its volume parameter. Target arrival clears the target,
not speed memory. A subsequent ordinary note does not clear the stored target.
Target direction and arrival tests use native signed-word comparisons even if
an earlier slide wrapped the stored period into the upper half of the word.

Tone-note targets use the pinned zero-finetune table, including its zero sentinel.
Ordinary reference notes still retain raw periods. A glide note never creates or
restarts a PCM voice; repeating its current instrument resets sample volume.
Different-instrument and slice-target handoffs are explicit preflight refusals
until their playback/loop semantics are implemented.

The existing 52-byte diagnostic is unchanged. The dev36 ramp oracle derives
trigger/reset decisions from fresh-row DMA-start masks, volume from raw volume
writes and rate from output-period writes. It checks the reference PCM policy,
not actual Paula sample timing. Same-instrument glide coverage starts at period404
so the sample is between loop boundaries at the glide; a forced-retrigger mutant
must fail that oracle. Old fixture evidence is retained unchanged.


## Arpeggio register behavior (dev37)

0xy runs only on effect passes and only with a nonzero parameter. The replay's
32-byte phase table is equivalent to `(counter & 31) % 3`: phase zero writes the
stored word, phase one uses the high nibble and phase two uses the low nibble.
Lookup finds the first zero-finetune period less than or equal to the unsigned
stored word, then adds the nibble offset. The stored word is never changed.
A zero nibble still performs lookup, which matters after a non-table pitch slide.
Fresh rows perform the ordinary PerNop write; delayed tick zero runs the effect.

The zero-finetune table includes a zero sentinel at index36, and native arpeggio
can read up to index51. These reads intentionally reach the following tuning+1
table. The portable implementation explicitly stores those 15 adjacent words;
it neither clamps the index nor reads beyond a C array. Finetune itself is still
unsupported. A sounding zero output period fails measurement before sink or
sample mutation. An inactive voice remains silent despite period writes.

The unchanged 52-byte native diagnostic and shared dev36 DMA/phase PCM oracle
cover nibble order, blank-command transition, delayed rows, maximum speed,
adjacent-table reads, zero refusal, prior slide/wrap and all four native tracks.
Host boundary checks also cover independent parameters on all 16 tracks and
track-mask isolation. Ordinary reference notes still retain raw periods.


## Vibrato output modulation (dev38)

4xy latches each nonzero nibble on effect passes: high for speed and low for
depth. Zero nibbles retain prior memory. 6xy calls the same modulation with that
memory unchanged, then applies its own volume-slide parameter. Fresh speed-one
rows can therefore leave a new 4xy parameter unused. Delayed tick-zero passes
execute modulation and advance phase just like other effect passes.

Vibrato phase is an unsigned wrapping byte, advanced by four times speed after
the period write. The low five bits of phase shifted right two index the pinned
32-entry sine table. E4x low bits select sine (0), ramp (1) or square (2 and 3).
The signed phase half determines addition/subtraction. Native ramp negative-half
magnitude is 255 minus eight times the index; square magnitude is 255. Multiply
by depth, shift right seven, then add/subtract with 16-bit wrap. The stored base
period is never modified and the PCM voice is never restarted by an effect pass.

E4x stores its low nibble on every visit. Bit2 inhibits vibrato phase reset on
ordinary notes, bit3 has no vibrato effect. A note checks the *previous* control
before its same-row E4 command runs. Tone-portamento targets do not reset phase.
Fresh 4xy/6xy rows restore the base period without modulating or latching their
parameters. Zero output on a sounding voice remains a preflight refusal.

The unchanged native 52-byte diagnostic supplies the output-period oracle;
reference ramp PCM uses those writes with native volume and fresh DMA starts.
This does not model Paula sample timing or analogue output.
