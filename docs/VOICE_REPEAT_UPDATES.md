# Live repeat-range preparation

`pt_voice_set_repeat` changes a live voice's next forward repeat range within
its existing immutable PCM. The current segment or loop iteration finishes at
its previous boundary; updating the range does not jump or restart its phase.
The last update wins if several arrive before the boundary. Interpolation at
the boundary blends into the newly selected repeat start. Position, step,
precision and interpolation setting are otherwise preserved.

This also lets a currently active one-shot acquire a forward repeat. Invalid,
empty or out-of-source ranges, inactive voices and pingpong voices are refused
without changing state. No PCM writes or allocation occur. The existing voice
contract requires valid initialized state and borrowed PCM throughout use.

Tests cover initial-segment updates, replacement before a boundary, updates
while already looping, one-shot interpolation across the new boundary and
invalid/inactive/pingpong refusal, alongside existing huge-step trajectories.

This is a foundation for reference repeat-register behavior. The renderer does
not yet call it or accept previously refused instrument-only sample/slice
handoffs. The original API describes only a range in the same source; the
additional borrowed-source API is described below. No new full
tracker-effect compatibility or physical audio acceptance is claimed.

Validation: the complete 72-test host suite passed. Native run `segment1789953011204947000` passed the updated segment and legacy mixer tests in 27.697 seconds, with identical host/native trajectory output and stopped DMA. Evidence is retained in `evidence/enhanced-editor/dev64`. Guarded emulator shutdown and fresh release checks completed.

## Borrowed next-source handoff (dev65)

`pt_voice_set_repeat_source` queues another immutable PCM for the next repeat
boundary. The current source and current iteration finish without a phase jump.
Boundary interpolation reads the new source's first repeat frame, then the
voice switches its borrowed source at the boundary. Any overshoot is folded
into the new repeat length. Further updates replace the pending source/range;
`pt_voice_set_repeat` can cancel a foreign-source handoff in favor of the
currently sounding source.

The pending source must have matching bit depth, channel count and sample rate.
This operation does not convert formats or change playback step. Both borrowed
source descriptions and PCM must remain valid and immutable while referenced.
Full source validation and voice/source alias checks precede mutation. Frame
and mix output checks protect pending data and metadata as well as current
data, even before the handoff occurs. No allocation or PCM writes occur.

Tests use different source lengths, verify interpolated forward and return
handoffs, pending-source cancellation, source immutability, incompatible-rate
and range refusals, and pending-source output alias refusal with unchanged
voice/output/clip counts. Existing segment trajectories and legacy mixing remain
in the suite. Renderer effect acceptance is still unchanged; pinned tracker
fixtures and integration are the next steps, not evidence established here.

Dev65 validation: all 72 host tests passed, as did the cross-build and native segment/legacy voice tests (`segment1789953593091072000`, 27.764 seconds). Native and host trajectory output matched. Evidence and guarded release checks are in `evidence/enhanced-editor/dev65`. No renderer integration or physical acceptance is claimed.

## Pinned active instrument-only fixtures (dev66)

Four two-sample MOD fixtures now capture looped handoff, return to the first
sample, handoff to a non-looping sample, and ED3 on an instrument-only row.
Each was run twice against the pinned sample-trace diagnostic; every trace byte
matched. The independent baseline's first 52 bytes per tick still match dev38.

The captures show the stored sample start/length, loop pointer/length and volume
change immediately at the instrument-only row. The trigger snapshot remains
the first sample at offset 2108, length 512 words, trigger count one throughout;
period stays 428. The second sample is at offset 4156. Its looping repeat is
[4220,4476), while its non-looping repeat is one word at 4156. ED3 without a
note does not generate a new trigger. Returning to the first instrument updates
the loop and volume again without retriggering. These properties are asserted
for every active tick, not just the row-transition snapshot.

The source's SetDMA writes each channel's stored loop pointer and repeat length
after processing a row, even with no newly triggered note (see pinned
`vendor/pt23f/PT2.3F.s`, SetDMA). These traces capture stored ranges and trigger
writes; they do not directly observe Paula's live fetch pointer or prove audible
PCM continuity. The next renderer comparison must model the current iteration
finishing before its new repeat source is used, and account for non-looping
first-word handling explicitly. Renderer refusal boundaries remain unchanged.

Native run `instrument1789954169827287000` passed nine executions in 37.299 seconds.
The new retained-evidence regression passed in 0.005 seconds; the previous
72-test suite remains the product-code baseline (no product changes here).
Evidence is under `evidence/enhanced-editor/dev66`. Emulator resources were
verified released and AmiConnect was notified. No physical acceptance claimed.

## Bounded renderer integration (dev67)

The renderer now accepts plain instrument-only changes between active mono8
forward-loop samples with equal rates, even classic sample/loop bounds, loops
of at least four frames and no interpolation. The current source iteration
finishes before the new repeat starts; volume changes at the row immediately.
Returning to the prior instrument follows the same rule.

Simultaneous effects on the handoff row, tracks containing 9xx/E9x/EDx, sliced
voices, non-looping targets, high-resolution/stereo sources, mismatched rates
and other unsupported combinations still fail in measurement before sink calls
or output-report mutation. These boundaries avoid implying support for untested
retrigger, sample-word-silence or format-conversion behavior.

A separate PCM oracle reads the pinned dev66 trace's loop/volume values and
walks the original MOD's signed bytes, holding the current iteration boundary
until reached. The renderer is checked sample-by-sample against it for handoff
and return fixtures; non-looping and ED3 variants remain refusals. The oracle
uses a declared one-source-frame-per-output-frame rate to isolate phase and
repeat behavior. This is reference-derived PCM, not captured analogue audio.

Dev67 validation: 74 host tests passed in 129.321 seconds, including the new
sanitized PCM oracle and existing main-screen golden and renderer refusal tests.
Final cross-build passed. Native run `handoffrender1789954937398364000` passed
four cases in 51.989 seconds: exact handoff/return PCM plus nonloop/ED refusal
before sink output. Stopped DMA and guarded release were verified. Evidence
is in `evidence/enhanced-editor/dev67`; physical acceptance remains outstanding.

## Instrument-only EDx handoff (dev68)

An EDx command on a row with an instrument but no note now follows the pinned
no-retrigger behavior during a compatible looped sample handoff. It changes
the next repeat source and immediate volume exactly like the plain handoff.
The captured ED3 fixture now passes the same independent PCM oracle as the
plain and return fixtures.

Only EDx with an actual period note participates in delayed-trigger range
tracking and the cross-sample delayed-note refusal. A note-free EDx cannot
trigger sample playback, so it no longer imposes that unrelated range limit.
Adding an actual delayed note to each fixture still refuses before any sink
call or report mutation. Depending on preflight order, the refusal may report
an effect or sample-range limitation; tests check both allowed refusal reasons.
The initial negative test expected only the effect code and was corrected to
include the existing sample-range refusal, without weakening the no-output and
no-mutation requirements. Non-looping handoffs and other unsupported combinations
remain refused.

Dev68 validation: 74 host tests passed in 131.955 seconds; final targeted sanitized PCM test passed in 0.789 seconds. Cross-build passed. Native run `handoffrender1789955548868698000` passed four cases in 61.766 seconds, including exact ED3 handoff PCM and actual delayed-note refusal on every fixture. Stopped DMA and guarded release verified; evidence is in `evidence/enhanced-editor/dev68`. No physical acceptance is claimed.

## Explicit volume on handoff rows (dev69)

Compatible looped instrument-only handoffs may also carry Cxx. The new sample's
repeat source is queued without restarting the current iteration, while Cxx
overrides the sample's default volume on that same row. Values above 64 clamp
to 64, following the pinned tracker. Other unimplemented simultaneous effects
retain their existing refusal boundaries.

Two new pinned fixtures combine a sample handoff with C10 and C7F. The retained
trace regression checks immediate volume (16 and clamped 64), unchanged trigger
count and repeatable captures. The separate PCM oracle checks every rendered
sample using those reference volume values, including the tail of the previous
sample before its boundary. The existing real delayed-note refusal assertions
run against these fixtures too.

Dev69 validation: 75 host tests passed in 131.520 seconds; targeted sanitized
trace/PCM validation passed in 0.947 seconds. Pinned capture run
`instrument1789956475324568000` passed five executions in 32.080 seconds,
with exact duplicate traces and baseline compatibility. Native renderer run
`handoffrender1789956573827654000` passed both PCM fixtures in 45.058 seconds,
including actual delayed-note refusal checks. Stopped DMA and separate guarded
release checks passed. Evidence is in `evidence/enhanced-editor/dev69`.
No physical or analogue acceptance is claimed.

## Volume slides on handoff rows (dev70)

Compatible looped instrument-only handoffs now also permit Axy, EAx and EBx.
The existing volume engine applies ordinary slides on later ticks and fine
slides on tick zero, while the queued sample repeat preserves the current
iteration. Four new pinned fixtures cover upward/downward ordinary and fine
slides. Trace assertions check each tick's volume and unchanged trigger count;
the independent PCM oracle checks the old-source tail and new repeat at those
volumes. Other format, offset/retrigger and unsupported-effect boundaries remain.

Dev70 validation: all 76 host tests passed. Targeted sanitized trace/PCM checks
passed in 0.863 seconds. Pinned capture `instrument1789957204222876000` passed
nine executions in 36.855 seconds; duplicates and the baseline matched. Native
renderer `handoffrender1789957330074380000` passed four PCM cases in 66.796
seconds, including actual delayed-note refusal assertions. Separate guarded
release checks and stopped DMA passed. Evidence is in `evidence/enhanced-editor/dev70`.
Physical availability remains unknown: AmiConnect reported no newer observation
since the earlier connection timeout. No physical contact or test occurred.

## Note cuts with phase recovery (dev71)

Compatible looped instrument-only handoffs may carry ECx. It cuts volume at
the requested tick without stopping or resetting the old iteration or new
repeat source. EC0 and EC3 reference fixtures restore volume with C40 on the
following row so the PCM comparison can detect a stopped or restarted voice,
including changes hidden while output was muted. Unsupported formats and
cross-sample actual delayed-note cases retain their refusal boundaries.

Dev71 validation: 77 host tests passed in 133.291 seconds; targeted sanitized
trace/PCM validation passed in 0.938 seconds. Pinned capture
`instrument1789957942754781000` passed five executions in 33.603 seconds with
matching duplicates and baseline. Native renderer
`handoffrender1789958061971517000` passed both cut/recovery PCM cases in
56.656 seconds, including delayed-note refusal checks. Separate guarded releases
and stopped DMA passed. Evidence is in `evidence/enhanced-editor/dev71`.
No physical or analogue acceptance is claimed.

## Canonical non-looping target (dev72)

A compatible active looped voice may now hand off to a mono8 non-looping sample
whose first two frames are already zero. The current iteration finishes, then
the original one-word repeat becomes silence. The renderer borrows those two
zero frames; it does not clear, rewrite or copy source PCM. This models the
canonical first-word condition established by original ProTracker loading.

Nonzero first words remain refused, as do unsupported formats, rates and effects.
A subsequent instrument-only handoff from the non-looping source also remains
refused pending a separate continuation model. A new ordinary note can still
trigger normally. The fixture deliberately retains audible later frames in the
non-looping sample: the PCM oracle checks that they are never started by the
instrument-only change. Old nonzero-first-word refusal tests remain in the suite.

Dev72 validation: all 78 host tests passed, including sanitized reference-derived
PCM and the retained nonzero-first-word refusal. The targeted test passed in
0.798 seconds; the cross-build passed. Pinned capture
`instrument1789958877274244000` passed three executions in 29.848 seconds with
identical duplicate traces and the unchanged baseline. Native renderer
`handoffrender1789958981703646000` passed exact PCM and delayed-note refusal in
35.801 seconds. Both windows ended with guarded cleanup, restored launcher and
fresh release checks. Evidence is in `evidence/enhanced-editor/dev72`.
This is software/emulator evidence, not live Paula audio or physical acceptance.

## Pitch slides during handoff (dev73)

Compatible instrument-only sample changes may carry 1xx, 2xx, E1x or E2x.
The new repeat source is queued while pitch changes retain the current playback
position and fractional phase. Existing sample/rate/offset/retrigger restrictions
remain. The reference PCM oracle now consumes captured output periods and walks
sample bytes using Q32 phase, including fractional overshoot at repeat boundaries.
The four fixtures distinguish tick slides from tick-zero fine slides in both
directions and assert that no extra sample trigger occurs.

The first capture attempt was stopped before launch by the build-manifest guard
while the build was still completing. A subsequent capture exposed an overlong
fixture title that shifted MOD bytes; the native loader refused it. The title
was shortened, fixture byte lengths/signatures checked, and the corrected
capture rerun in a separately coordinated window. Neither failed attempt is
counted as validation success; failure logs and release evidence are retained.

Dev73 validation: 79 host tests passed; targeted sanitized trace/PCM checks
passed in 0.842 seconds. The cross-build passed. Corrected pinned capture
`instrument1789959714164034000` passed nine executions in 36.882 seconds,
with duplicate/baseline matches. Native renderer
`handoffrender1789959823678263000` passed all four PCM cases and negative
checks in 74.477 seconds. Final fresh release was 03:05:26 UTC, with no emulator
claim retained. Evidence: `evidence/enhanced-editor/dev73`. No physical or
analogue audio acceptance is claimed; the clock remains ideal BPM/Q32.

## Modulation during handoff (dev74)

Compatible instrument-only handoffs support arpeggio, vibrato, vibrato with
volume slide and tremolo (0xy/4xy/6xy/7xy). Playback phase continues across the
queued source change while period and volume follow the pinned reference.
The 6xy fixture establishes vibrato memory before changing instrument, checking
continued modulation rather than a reset. Existing format/rate, sliced voice,
offset/retrigger and actual delayed-note refusal boundaries remain.

Dev74 validation: 80 host tests passed in 137.212 seconds; targeted sanitized
trace/PCM checks passed in 0.861 seconds; cross-build passed. Pinned reference
`instrument1789960433603404000` passed nine executions in 36.856 seconds with
identical duplicate traces and baseline. Native renderer
`handoffrender1789960553310914000` passed all four exact PCM cases and
negative checks in 75.324 seconds. Both windows were separately coordinated,
guarded and released; final fresh release 03:17:41 UTC. Evidence lives in
`evidence/enhanced-editor/dev74`. No physical or analogue acceptance is claimed.

## Instrument-only tone portamento handoffs (dev75)

Compatible instrument-only 3xx/5xy rows may switch the queued repeat source
while retaining an established tone target and speed. Four fixtures establish
an upward/downward target before switching sample, with and without volume
slide. Cross-sample tone rows containing a new note remain refused; only the
instrument-only case has been opened. Existing rate/format/slice/offset and
retrigger restrictions still apply.

Dev75 validation: 81 host tests passed in 138.468 seconds; targeted sanitized
trace/PCM validation passed in 0.709 seconds; cross-build passed. Pinned capture
`instrument1789961169027320000` passed nine executions in 39.697 seconds with
identical duplicates and baseline. Native renderer
`handoffrender1789961281700759000` passed four PCM cases and delayed-note
refusal checks in 99.975 seconds. Both emulator windows were separately
coordinated and released; final fresh release 03:30:13 UTC. Evidence is in
`evidence/enhanced-editor/dev75`. No physical or analogue acceptance claimed.

## Tone targets on cross-sample rows (dev76)

The bounded handoff also applies when a 3xx/5xy row supplies a new target note.
It queues the new sample repeat, applies the target and retains current voice
phase rather than triggering that note. Four fixtures establish speed first,
then combine upward/downward target notes with a sample change, optionally
sliding volume. The same classic-format/rate/slice/offset/retrigger guards apply
before rendering; actual delayed-note EDx remains refused.

Dev76 validation: 82 host tests passed in 138.885 seconds; targeted sanitized
trace/PCM checks passed in 0.733 seconds; cross-build passed. Pinned capture
`instrument1789961911775418000` passed nine executions in 39.222 seconds with
identical duplicate/baseline traces. Native renderer
`handoffrender1789962021177207000` passed four PCM cases and delayed-note
refusal checks in 101.019 seconds. Both windows were separately coordinated,
guarded and released; final fresh release 03:42:27 UTC. Evidence is in
`evidence/enhanced-editor/dev76`. No physical or analogue acceptance claimed.
