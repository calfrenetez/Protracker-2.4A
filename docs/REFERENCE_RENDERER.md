# Bounded reference renderer and sample bounce (dev31–33)

`PT24GRender` is a native/host command-line renderer built from the shared portable
flow, frame-clock and voice/mix cores. It accepts validated MOD, PP20 MOD and PTG
projects and writes a new stereo PCM WAV. It uses bounded streaming storage and
never requires a full rendered song in RAM. This is the first supported subset,
not complete classic-effect playback or a model of analogue/hardware sound.

## Commands

```text
Stack 65536
PT24GRender input.mod new-song.wav
PT24GRender input.ptg new-pattern.wav --pattern 0 --bits 24 --rate 48000
PT24GRender input.ptg selected.wav --tracks 000F --gain 65536 --bits 16 --rate 44100
```

Host build: `make renderer`, then use `build/host/PT24GRender` with the same
arguments (without the Amiga `Stack` command).

Defaults are 48000 Hz, stereo24, all tracks, Q16 master gain 32768 (one half),
startup silence trimmed, a 30-minute internal frame budget and one million ticks.
`--gain 65536` selects unity; zero is allowed. The tool reports clipped output
sample values. It does not normalize automatically or apply dither. Track masks
are hexadecimal, with bit 0 selecting channel 1. `--lead-in` retains the initial
speed-count silence. Existing output paths are always refused.

Input files and decoded document storage each have a 64 MiB policy. Allocation
failure remains possible and fails before publication. Output size is bounded to
signed32 file length; the time/tick budgets can refuse earlier. The frame budget
includes the internal startup lead-in even when that silence is trimmed from the
published WAV. Real 68000/68030 completion time and memory headroom need physical
measurement; native emulator success is not a performance guarantee.

## Editor controls (dev32)

Open **DISK OP. → RENDER WAV**, or press **Control-Shift-W**. This uses the
same reference renderer and verified new-file publication as `PT24GRender`.
The main tracker layout is unchanged.

| Control | Key | Meaning |
| --- | --- | --- |
| SONG / PATTERN | P | Full song from order zero, or the current pattern; clears marked-row mode |
| MARKED ROWS | E | Snapshot marked rows/tracks from the current pattern |
| TRACKS | M | Nonzero hexadecimal mask of available tracks |
| ALL / ONE | A / T | All tracks, or the currently selected track |
| 44.1 / 48 KHZ | R | Output rate |
| 16 / 24 BIT | B | Output precision |
| GAIN | G | Cycle 50%, 100%, 25% master gain |
| LEAD IN | L | Retain or trim initial speed-count silence |
| WAV FILE | W / Return | Preflight, choose a new destination, render and verify |
| NEW SAMPLE | U | Render into a new assignable sample slot as one undo step |
| STEMS | S | Export selected track/group WAVs to a new folder |
| TRACK / GROUP STEMS | O | Toggle individual tracks or nonzero logical groups |
| BACK | Escape | Return to disk operations |

Defaults match the CLI: song, all tracks, 48 kHz, 24 bit, 50% gain, trimmed
lead-in. Settings are transient preferences and do not change the project or
consume undo history. Global mute/solo still apply to selected tracks; selection
does not bypass unsupported MIDI routes. A zero or out-of-range mask is refused.

Rendering stops Paula playback before preflight. Unsupported data is refused
before opening the file requester. During checking, mixing and byte verification,
Escape cancels; other editor input is ignored while the renderer borrows immutable
project/sample data. The window continues to process refresh messages. Progress
shows the active stage and, for mixing/verification, percentage of measured frames.
A cancelled or failed render leaves the project intact and removes its own staging.
Existing destination files are never replaced. Exporting does not mark unsaved
project edits saved. Successful clipped output is reported with advice to lower gain.

This is supported-subset offline rendering. Complete classic effects and hardware-equivalent audio remain unfinished.
Batch stems are described below.

## Defined reference behavior

Timing is `IDEAL_BPM_Q32`, the deterministic 2.5/BPM clock described in
[the offline replay design](OFFLINE_REPLAY_DESIGN.md). Pitch policy is
`PCM_RATE_PERIOD428`: sample PCM rate plays at period 428 (display C-2), and other
periods scale by 428/period. This preserves high-resolution sample-rate intent;
it is not a PAL/NTSC Paula clock, CIA timer latch or analogue filter model. The
CLI prints these profiles and selected rate/precision/tracks/gain/lead-in policy.

The first F00 ends rendering. Otherwise one pass ends on the first position
transition to the same or a lower order, after the outgoing row has played its
full duration. This includes a normal song wrap or backward Bxx restart. E6 row
loops are not position transitions and retain their repetitions. Pattern mode
uses a one-entry order list pointing at the selected pattern. Budget exhaustion
is an error, never silently classified as a completed song.

Supported: ordinary native-table-quantized notes (including instrument-zero inheritance),
explicit note-off and velocity; mono/stereo 8/16/24-bit samples; nearest/linear
interpolation; forward/ping-pong loops; sample slices; track selection; global
mute/solo; and effects `0xy`, `1xx`, `2xx`, `3xx`, `4xy`, `5xx`, `6xy`, `7xy`, `8xx`, `9xx`, `Axx`, `Bxx`, `Cxx`, `Dxx`, `E1x`, `E2x`, `E3x`, `E4x`, `E5x`, `E6x`, `E7x`, `E8x`, `E9x`, `EAx`, `EBx`, `ECx`, `EDx`, `EEx`, `Fxx`.
The reference treats `8xx` as unused: its parameter does not pan audio, but the
replay restores the stored pitch on fresh and effect ticks. `E8x` is a true
no-op because the pinned2.3F replay removed Karplus-Strong processing. It can
retain the final vibrato/slide output period until a later register write; it
must not be replaced with a generic period restore. Neither changes sample data.
Filter `E0x` and invert-loop `EFx` remain refused.

Arpeggio 0xy changes the output period on effect passes while preserving the
stored base and PCM phase. Its tick cycle uses the original masked counter;
zero nibbles still perform table lookup, whereas command 000 does not run an
effect. High offsets retain the original adjacent tuning-table values. A
sounding zero result is refused during measurement, before output.
Vibrato 4xy remembers each nonzero speed/depth nibble independently. 6xy uses
that memory while sliding volume. E4x selects sine, ramp or square waveform and
whether ordinary note triggers reset vibrato phase; native selections 2 and 3
both produce square. Tone-portamento notes preserve vibrato phase. Output-period
modulation wraps as a native word and leaves the stored period and PCM phase intact.
Axx applies on effect passes, including delayed passes. Fine volume slides EAx/EBx
apply only at tick zero, including delayed tick-zero passes, and saturate at 0/64.
ECx cuts volume when the current tick equals x; EC0 is immediate and an x outside
the row speed never fires. Cutting preserves voice phase, so later Cxx or a volume
slide can restore the continuing sample. These semantics are checked against
repeated raw-volume traces from the pinned 2.3F replay code (dev34).
Global flow commands on tracks excluded from audio still control the song. Selected slice ranges use
one-shot playback unless the complete sample loop lies within that slice.

Pitch slides 1xx/2xx run on effect passes, including delayed passes; a zero
parameter is literal zero, not effect memory. Fine slides E1x/E2x run only on
tick zero, including delayed tick zero. The interpreter retains the stored
16-bit word and the last playback-period write separately. It preserves the
original replay's wrapping arithmetic, 12-bit masking on slide writes, low-12
limits of 113/856, and later full-word writes from PerNop/SetBack. Slide updates
change the rate without restarting the voice or resetting its fractional phase.

Tone portamento 3xx sets a target without restarting the voice. Its nonzero speed
parameter is remembered on effect passes; 300 reuses that speed. 5xx keeps the
remembered glide speed and adds normal volume sliding, including high-nibble
priority and clamping. Delayed effect passes retain this behavior. Arrival clears
the target while retaining speed memory. Normal note triggers do not clear a
pending target. Target selection and comparisons follow the active pinned finetune
period table and signed 16-bit replay arithmetic, including wrapped stored words.

Repeating the current sample number on a glide note resets its volume without
retriggering. A glide before the first ordinary note does not start a voice.
Explicit project velocity on a glide note updates velocity without retriggering.
Cross-sample glide handoff and slice targets are refused before output; their
playback/loop handoff is not approximated. These are current implementation limits.

Measurement refuses a triggered voice whose period becomes zero, before any sink,
WAV staging or sample append. Zero-period playback is an explicit remaining
reference limitation. Ordinary notes now use native table quantization and
finetune; the reference rate policy still does not model PAL clock/analogue behavior.
Separate stored/output register traces and ramp PCM comparisons are in dev35
and dev36; dev36 also checks native DMA-trigger continuity and glide memory.

Instrument-only rows can preload a sample before any note sounds, or reload the
same whole sample's stored volume/finetune without restarting its voice phase.
Offset range memory reload, explicit E9 retrigger and delayed-row volume changes
retain their native ordering. A different sample on an active voice, or reloading
an active slice, still requires a separate pending DMA/repeat-source model and is
refused during measurement before output. Instrument-only slice selection and
instrument-bearing OFF events remain refused. Native dev54 fixtures and true24
track16 boundary tests cover the supported phase/volume behavior.

Mono pan uses a linear left/right split over 0..255. Stereo pan uses balance:
centre128 leaves both sides at unity, and each endpoint silences the opposite
side. Gains are Q16, sample products accumulate in signed64 and final output is
rounded/quantized/saturated. Muting or zero gain does not stop voice progression.
Sample data is never modified, including across file verification passes.

Preflight refuses selected MIDI routes (external audio is absent), MIDI pitches,
crossfade-loop metadata, active cross-sample/slice instrument-only handoffs and other
effects. These are explicit implementation limits. It checks all order-referenced
patterns conservatively (or only the selected pattern in pattern mode), before
creating staging or calling the output sink. Excluded audio tracks still retain
their supported global flow behavior. Unfinished commands are not approximated.

## Publication and cancellation

The utility measures the bounded render, creates a uniquely owned temporary file
on the destination filesystem, streams WAV bytes and closes it. It then renders
again and compares every staged byte, including the header and exact file end.
Only after success does it publish through the established POSIX link/Amiga DOS
Rename no-replace policy. Cancellation, short writes, malformed output, failure to
close/read/verify, and destination races remove only this operation's staging.
A file that appears at the destination during rendering is preserved.

The library progress callback distinguishes analysis, mixing and verification;
returning zero cancels. Streaming blocks are at most 256 stereo frames and their
pointers are temporary. The project, PCM and options must remain immutable for
all passes. The result report is replaced only after successful completion. The
CLI currently has no interactive cancel key; editor progress/cancel controls and
render-to-sample journal integration remain subsequent work.

Tests cover exact true24 WAV bytes, host/native reference parity, startup trimming,
complete row endings, delayed phase continuity, slice bounds, 16-bit rounding,
volume/velocity/note-off, global mute/solo, excluded-track flow commands, unsupported
preflight, capacity, cancellation, corrupt staging, partial file writes, destination
races and staging cleanup. No physical A1200/AmiGUS is involved. Native provenance
and current acceptance are retained under `evidence/enhanced-editor/dev31/`.


## Render to a new sample (dev33)

On the render page, **NEW SAMPLE / U** uses the displayed song/pattern, track mask,
rate, precision, gain and lead-in settings. The result becomes the selected new
sample, named BOUNCE SONG or BOUNCE PATTERN nnn. Existing instruments and events
are preserved. The sample retains stereo16/24 PCM at 44.1/48 kHz, volume 64,
finetune zero, and no automatic loop or slices. Assignment to a note is explicit.

Rendering uses one new PCM allocation directly, without an intermediate WAV or a
second full PCM copy. This allocation, sample metadata, undo ownership and table
growth are charged to the existing 32 MiB sampler budget. Available memory or the
255-slot limit can refuse a bounce. No automatic precision/rate reduction is made.

Adding the completed sample is one shared undo command. Escape during checking or
mixing—including the final callback before commit—leaves the project, sample
count and existing redo branch unchanged. Later edits to the bounced sample join
the same chronological history. Undo cannot remove a still-referenced slot; undo
its note assignment first. PCM remains owned when its append command is evicted.

Control-L opens the selected bounced sample for editing/export. High-resolution
Paula audition remains unsupported; bouncing does not imply AmiGUS live playback
or physical audio acceptance. Selected row-range bounce is not yet implemented.


## Sample offset subset (dev41)

9xx uses per-track offset memory in 256-byte units. A note row applies the offset
before capturing its initial playback range and again to its saved range, matching
the pinned replay. 900 reuses memory. No-note 9xx changes only the saved range,
once on fresh fetch; delay passes do not repeat it. Instrument reload restores
ranges but keeps offset memory. At/beyond remaining length, the saved length
becomes two frames without advancing start. Initial segments can hand off to
forward loops outside their range. Later inherited notes use the saved range.

For each selected track containing 9xx in the chosen song/pattern scope, all
referenced samples must be mono8, nonempty even lengths from 2 to 131070 frames,
and have no loop or an even-boundary forward loop. Slice-trigger notes on those
tracks are refused. Stereo, higher precision, odd lengths and pingpong offset
semantics remain unsupported and fail preflight before sinks or publication.
Unselected tracks and unused patterns do not impose this audio restriction.

The immutable reference policy remains in force: nonloops stop after their
initial segment, and sample data is not rewritten. Native replay's first-word
clearing and repeated silent guard are not emulated. Reference PCM is checked
against native range/trigger snapshots, not analogue Paula output.

## Retrigger and note-delay reference evidence (dev42)

At dev42, E9x/EDx remained refused by renderer preflight. The separate native sample diagnostic
now has ten repeated fixtures covering their trigger counts and initial/loop ranges.
E90 never retriggers. E9x with x nonzero triggers when the counter is divisible by
x, except counter zero with a packed note; that note already takes the ordinary
trigger path. A no-note E9x does retrigger at zero. Pattern-delay passes retain the
packed note, so the counter-zero exception also applies on repeated passes.

EDx suppresses the ordinary note trigger. It triggers only with a packed note and
a counter equal to x: ED0 triggers at zero; an amount at/above speed never fires.
Pattern delays can fire the retained note again. Both effects use the saved start
and word length, including the second saved offset application from a prior 9xx
note, and preserve the independent loop range. Explicit expected trigger ticks
and an independent range model check every 140-byte native record.

Evidence is under evidence/enhanced-editor/dev42. The existing first52 diagnostic
fields retain exact baseline parity. No shipping code or layout changed. These
register/trigger captures are not rendered-PCM or physical listening acceptance.
Next: integrate trigger scheduling into both measurement and streaming, handle
EDx stored-period versus hardware-period timing and vibrato reset semantics, and
compare PCM against these retained native triggers before enabling either effect.

## Retrigger rendering subset (dev43)

E9x is now enabled in the shared WAV and new-sample renderer. Nonzero x restarts
the saved sample segment on divisible counters, excluding counter zero when the
retained row contains a note. E90 has no additional trigger. Fresh notes retain
ordinary trigger behavior; no-note retriggers preserve velocity and volume.
Retriggers restore the stored period without resetting vibrato phase. Offset
memory and independent loop ranges are retained. Measurement checks the same
ranges and zero-period restrictions before any output is published.

Selected tracks containing E9x use the same bounded mono8/even-length/no-slice
subset as 9xx, including E90. Higher-precision, stereo and pingpong retrigger
semantics remain unsupported. EDx note delay was still refused at dev43. Five retained
dev42 fixtures check 46080 rendered frames against native trigger/range evidence
on the host and native emulator; channel16 and offset-then-retrigger cases have additional checks. All six native checks match host output.
This does not establish physical playback or complete classic compatibility.

## Note-delay rendering subset (dev44)

EDx now defers the sample trigger until counter x. ED0 starts at counter zero;
no-note EDx never triggers, and a delay beyond the row speed never fires. Pattern
delay retains the note and can trigger it again. Fresh EDx stores the native
zero-finetune table period, keeps the previous hardware-output period until the
trigger, and does not reset vibrato phase. Instrument volume updates on row fetch;
explicit note velocity takes effect on the delayed trigger.

The same selected-track mono8/even/no-slice restrictions as 9xx/E9x apply. A delayed
change to a different sample while a voice is sounding is refused during measure,
before output; old/new sample handoff is not yet supported. Unsupported zero
periods still fail before publication. Six dev42 fixtures independently check
63360 reference frames, including offsets, forward loops and pattern delays.
Additional state checks cover delayed pitch writes, retained vibrato phase and
refusal of cross-sample delay. No analogue or physical acceptance is implied.

## Tremolo diagnostic preparation (dev45)

At dev45, 7xy/E7x were not yet enabled in the reference renderer. The separate 76-byte
PTVolumeTraceTest retains the exact existing 52-byte prefix. Prefix bytes32..35
already contain the final output volume before mute gating, captured by the
existing pt_write_volume wrapper at every volume write. New bytes52..59 contain
four big-endian stored-volume words; bytes60..75 contain command, tremolo phase,
full wave control and vibrato phase for each channel. No shipping playback hooks
are changed, and no additional volume-write wrapper is required.

The pinned replay updates nonzero speed/depth nibbles independently on nonfresh
7xy passes, clamps output to 0..64 without changing stored volume, and advances
tremolo phase by speed*4. Sine, ramp and both square selectors are distinct test
cases. The ramp magnitude chooses its branch using vibrato phase, while the
volume addition/subtraction sign uses tremolo phase. Preserve this native
cross-effect behavior. Wave control bit6 suppresses note-trigger phase reset;
pattern delays retain commands and execute nonfresh passes at counter zero.

## Tremolo rendering (dev46)

7xy/E7x now modulate a separate output-volume value, preserving stored sample
volume for subsequent commands. Nonzero parameter nibbles update independently;
700 memory, sine/ramp/square controls, phase reset suppression and pattern-delay
passes follow the captured native behavior. The ramp uses vibrato phase for its
magnitude branch and tremolo phase for volume addition/subtraction. Fresh7 restores
the stored pitch through PerNop; nonfresh tremolo does not write pitch.

The PCM oracle uses dev45 volume/pitch events and note-trigger flags, with the
immutable 2048-frame source ramp extended into a full-sample reference loop at
48 kHz so later effect ticks remain audible. All four waveform channels are
checked independently. This compares reference PCM policy against native control
state; it is not an analogue Paula waveform comparison or physical acceptance.

## Glissando rendering (dev47)

E3x controls stepped pitch output during 3xx/5xx portamento. Any nonzero low
nibble enables glissando; E30 disables it. The stored period still moves smoothly,
retaining slide speed and target; only each portamento pitch write is quantized
against the pinned zero-finetune table. Reaching the target preserves native
arrival behavior. Control changes do not immediately rewrite the period register.
At dev47 finetuned samples were refused; dev48 adds tuning. Undefined zero-output periods are still
rejected during measurement before any sink or publication.

## Finetune and ordinary-note quantization (dev48)

All16 sample tunings and E5x use the pinned native tables. Ordinary packed periods
first select an index in the zero-tuning table, then read that index in the active
tuning table; raw non-table periods are no longer passed through unchanged.
Sample reload restores its header tuning. E5x overrides tuning before a same-row
note and can change tuning without a note, without rewriting the current period.

Tone targets scan the active table and apply the native one-entry correction for
negative tuning. Glissando quantizes output against the active table without that
target correction. Arpeggio includes adjacent-table overflow and the15 explicit
overflow words after tuning -1. The generated table is checked byte-for-byte
against the pinned assembly. Zero output periods and cross-sample/slice handoff
limits remain; this does not change the reference sample-rate/ideal-BPM policy.

## Batch stems (dev49)

```text
PT24GRender input.ptg NEW-DIRECTORY --stems
PT24GRender input.ptg NEW-DIRECTORY --groups --tracks 00FF --bits 24
```

`--stems` writes one stereo WAV per selected track (`track-01.wav` etc.).
`--groups` combines selected members of each nonzero logical group into
`group-01.wav` etc.; group0 tracks remain individual. Selection never expands to
unselected group members. Order follows the first selected track in each stem.
The output argument is an entirely new directory, not a WAV filename.

All outputs keep global pattern/song flow, tempo, mute/solo, pan, gain and the
same start/end timing. A muted track therefore produces an aligned silent stem.
Selected MIDI routes are refused, including muted MIDI tracks; exclude them
explicitly or supply captured audio through a separate workflow. Separate
quantization/clipping means summing exported stems need not reproduce a master
bit-for-bit. The tool reports each stem's mask, frame count and clipped values.

Every selected stem is preflighted before staging begins. WAVs are individually
re-rendered and byte-verified inside an exclusively created temporary folder.
The whole folder is published with a no-replace rename only after all outputs
succeed. Failure/cancellation cleans up owned staging; an existing destination
or one created during the render remains intact. Cleanup failures are reported
with the retained staging path. A crash can leave an unpublished staging folder.
The project and source samples remain immutable, and memory use is bounded
independently of batch duration. The native render panel exposes the same batch operation through STEMS (S).
TRACK STEMS / GROUP STEMS (O) chooses the grouping mode; settings are transient
and do not alter project data or undo history. Enter a new folder name in the
requester. Escape cancels before publication and preserves unsaved edits.

### Emulator filesystem validation boundary

The editor stem workflow passes on native Amiga RAM: under the pinned emulator,
including cancellation cleanup and exact WAV/project checks. On its PTDEV host
shared folder, empty cancelled staging directories can reappear during a later
batch despite native and host checks initially reporting deletion. Correct WAV
output was verified, but shared-folder cleanup acceptance remains open. The
cause is not established; evidence is retained in dev50. No forced cleanup or
product workaround is applied. This is distinct from real-device acceptance.

## Selected rows in a pattern (dev51)

```text
PT24GRender input.ptg rows.wav --pattern 0 --from-row 8 --to-row 16
```

Row arguments are decimal; the interval includes row8 and excludes row16.
`--from-row` alone defaults the end to64; `--to-row` alone starts at0.
Bounds require0 <= first < end <=64, explicit pattern mode and no lead-in.
These options also apply to CLI stems. The native render panel also supports marked-row WAV, stem and sample-bounce output.

Playback pre-rolls from row0, advancing samples and effects before retaining any
output. Capture begins at the first fetched row within the interval, preserving
inherited sample, phase and effect memory. It ends before the first subsequent
fresh row outside the interval, or at the normal F00/position-return boundary.
Delays and loops wholly inside the interval remain; a jump below its start ends
the selection. Rows skipped by flow are not invented. If playback never reaches
a selected row, rendering refuses before any sink or file creation.

Pre-roll consumes the normal tick/frame budget and remains cancellable, while
reported output frames and clipping count only the captured interval. Preflight
still checks the entire selected pattern, including unused later rows. Source
patterns are immutable; this is a timed excerpt, not a rewritten pattern.

## Editor marked rows (dev52)

Mark a block in the pattern editor (Control-B, then move the cursor). Open
RENDER WAV and press E or click MARKED ROWS. The panel snapshots the normalized
half-open rows and tracks, switches to pattern scope and turns lead-in off.
Its heading displays the inclusive hexadecimal row bounds. The track mask can
then be refined using M, A or T. WAV FILE, STEMS and NEW SAMPLE all use this same
selection and state-preserving pre-roll. Bounced samples are named for the rows.

A missing selection or one from another pattern is refused without changing
settings. Cursor/mark movement after activation does not change the snapshot.
Changing to a different pattern makes the stored range invalid rather than
silently exporting different content. E disables range mode; P switches normal
song/pattern scope and clears it. Lead-in cannot be enabled while range mode is
active. Settings consume no undo history; a completed bounce remains one shared
undo step, and a cancelled bounce preserves the redo branch.

## Full native render workflow refresh (dev58)

The complete editor workflow now tests a supported finetuned sample rather than
expecting the pre-dev48 finetune refusal. The retained run
`renderui1789947391737397000` passed in 294.651 seconds: file-request cancellation,
cancellation during mixing and verification, exact song/pattern WAV agreement,
finetune +1 producing a different WAV that exactly matches the host reference,
stopped Paula DMA, and undo/save restoring the original PTG bytes. Finetune
export preserves the unsaved-edit state until explicitly undone or saved.

Evidence is under `evidence/enhanced-editor/dev58`. The retained-native host test
rebuilds the current renderer with sanitizers and compares the complete WAV bytes
for both original and tuned fixtures, plus saved-project identity. This extends
software/emulator workflow evidence; it is not physical A1200 or AmiGUS proof.
The native program code is unchanged from dev57. The runner checks source/binary
hashes before launch and uses guarded cleanup even when initial socket discovery
fails. The package readme no longer describes already-implemented group stems as
pending; enhanced live MIDI/panning remains unfinished.
