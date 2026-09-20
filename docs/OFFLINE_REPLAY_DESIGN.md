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
