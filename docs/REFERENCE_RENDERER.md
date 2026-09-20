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
| SONG / PATTERN | P | Full song from order zero, or the current pattern |
| TRACKS | M | Nonzero hexadecimal mask of available tracks |
| ALL / ONE | A / T | All tracks, or the currently selected track |
| 44.1 / 48 KHZ | R | Output rate |
| 16 / 24 BIT | B | Output precision |
| GAIN | G | Cycle 50%, 100%, 25% master gain |
| LEAD IN | L | Retain or trim initial speed-count silence |
| WAV FILE | W / Return | Preflight, choose a new destination, render and verify |
| NEW SAMPLE | U | Render into a new assignable sample slot as one undo step |
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

This is supported-subset offline rendering. Batch stems, complete classic effects
and hardware-equivalent audio remain unfinished.

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

Supported: ordinary raw-period notes (including instrument-zero inheritance),
explicit note-off and velocity; mono/stereo 8/16/24-bit samples; nearest/linear
interpolation; forward/ping-pong loops; sample slices; track selection; global
mute/solo; and effects `0xy`, `1xx`, `2xx`, `3xx`, `4xy`, `5xx`, `6xy`, `9xx`, `Axx`, `Bxx`, `Cxx`, `Dxx`, `E1x`, `E2x`, `E4x`, `E6x`, `EAx`, `EBx`, `ECx`, `EEx`, `Fxx`.
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
pending target. Target selection and comparisons follow the pinned zero-finetune
period table and signed 16-bit replay arithmetic, including wrapped stored words.

Repeating the current sample number on a glide note resets its volume without
retriggering. A glide before the first ordinary note does not start a voice.
Explicit project velocity on a glide note updates velocity without retriggering.
Cross-sample glide handoff and slice targets are refused before output; their
playback/loop handoff is not approximated. These are current implementation limits.

Measurement refuses a triggered voice whose period becomes zero, before any sink,
WAV staging or sample append. Zero-period playback is an explicit remaining
reference limitation. Notes still use their raw project periods; this does not
add ordinary-note table quantization, finetune or PAL clock/analogue behavior.
Separate stored/output register traces and ramp PCM comparisons are in dev35
and dev36; dev36 also checks native DMA-trigger continuity and glide memory.

Mono pan uses a linear left/right split over 0..255. Stereo pan uses balance:
centre128 leaves both sides at unity, and each endpoint silences the opposite
side. Gains are Q16, sample products accumulate in signed64 and final output is
rounded/quantized/saturated. Muting or zero gain does not stop voice progression.
Sample data is never modified, including across file verification passes.

Preflight refuses selected MIDI routes (external audio is absent), MIDI pitches,
nonzero sample finetune, crossfade-loop metadata, instrument-only events and other
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
