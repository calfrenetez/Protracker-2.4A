# Shared sequencer and offline replay: implementation boundary

This is the next implementation design, not an implemented renderer or an audio
acceptance claim. It uses the pinned local 2.3F CIA replay source at
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
shared strict MOD parser. These are **input fixtures only**: native traces and
portable-sequencer parity are NOT RUN, and the generated manifest says so. The
tool refuses an existing output directory so it cannot replace prior evidence.

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
