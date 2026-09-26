# ProTracker 2.4G — AmiGUS Edition

Build preparation for a native Amiga tracker with 1–16 channels, classic
ProTracker workflow, and exclusive Paula, AmiGUS or MIDI routing per channel.

The shared WAV/sample-bounce renderer now includes reference arpeggio (0xy),
finetune and native note quantization, pitch slides, tone portamento with glissando, vibrato and tremolo with waveform controls and mono8 sample-offset/retrigger/note-delay subsets. Instrument-only preload and
same-sample volume reload preserve phase. Bounded EFx invert-loop works in offline
WAV, selected/group stems and sample bounce using private copies of mono8
forward-loop samples; source masters remain unchanged. Full classic-effect compatibility remains in
progress; see [reference renderer limits](docs/REFERENCE_RENDERER.md).

**Development status: the native editor, sampler and song arrangement work;
full mixed-backend playback is unfinished.** Two Amiga executables are retained:

- `PT24GEdit` is the enhanced native prototype: the accepted classic screen/font,
  1–16-channel projects, four-column paging, note/block editing, shared undo,
  channel settings, sampler and song-position editing.
- `PT2.4G` is the separate four-channel assembler derivative (dev2), with MOD
  preflight and input.device mouse support. It remains the classic baseline.

The enhanced editor loads ordinary/PP20 MODs and versioned projects, creates new
songs, edits titles and channel/sample metadata, and saves verified new files.
POS ED. adds blank patterns and inserts/removes/reorders positions. Undo includes
notes, song structure, channel settings and sample edits. Supported classic songs
export losslessly to MOD; incompatible enhanced data is explicitly refused.
DISK OP -> CONVERT MOD and the standalone converter offer explicit [precision-only 8-bit conversion](docs/CLASSIC_PRECISION_CONVERSION.md)
for otherwise-classic 16/24-bit mono projects, preserving the source.

Classic four-channel Paula song/pattern playback, compatible sample audition,
live note edits and mute/solo work in the private Amiberry setup. Position changes
stop the old snapshot and Play restarts with the new sequence. P/A/M routing,
pan, groups, MIDI assignments and slices are saved settings; enhanced panning,
slice playback and mixed AmiGUS/MIDI output are still open.

Sampler +SMP adds slots up to 255 through the shared undo journal.
The sampler imports/exports PCM WAV, supported IFF/8SVX and explicitly configured
RAW, and imports selected instruments from ordinary/PP20 MODs. It offers range
selection, zoom, exact frame entry, reverse/normalize/gain/fades/DC removal,
loop metadata, baked crossfade, manual/AUTO SLICE proposals, shared immutable
PCM for metadata history, and explicit precision/rate conversion. Filtered
conversion provides progress and cancellation. High-resolution samples are
editable/saveable; Paula audition requires compatible sample formats.

A portable row/tick flow core now matches 16 pinned native replay traces on the
host and emulated 68030. The reference timeline and high-resolution mixer now
power `PT24GRender`, a bounded, supported-subset WAV utility with verified new-file
publication, now available from DISK OP. → RENDER WAV or Control-Shift-W.
NEW SAMPLE / U now bounces into a new sample slot with shared undo.
Fine volume slides, note cut, normal/fine pitch slides and tone portamento with
volume sliding are checked against repeated pinned native traces, including
stored-period wrap, delayed passes and sample continuity.
Selected-row WAV/bounce and track/group stems are implemented. Full effect
compatibility remains unfinished.
DISK OP. → RECENT provides a persistent ten-project history; see
[Recent Projects](docs/RECENT_PROJECTS.md).
See [renderer usage and limits](docs/REFERENCE_RENDERER.md) and
[replay design and evidence](docs/OFFLINE_REPLAY_DESIGN.md).

Host sanitizer tests and native emulator workflows retain source/binary IDs,
exact file comparisons, allocation-failure and undo checks, screenshots and
clean-exit evidence. MIDI ownership and quantised-recording cores also pass
host/native sink tests, but CAMD and live recording transport are unconnected.
Physical AmiGUS and ACA1234 performance/audio acceptance remain untested while
the owner's Mini is in transit. No emulator result substitutes for that gate.

See the [editor guide](docs/ENHANCED_EDITOR.md),
[software milestones](docs/HARDWARE_INDEPENDENT.md),
[Paula replay boundaries](docs/PAULA_REPLAY.md),
[MIDI/recording design](docs/MIDI_RECORDING_CORE.md),
[baseline report](docs/BASELINE_BUILD.md),
[diagnostic evidence](docs/AMIGUS_DIAGNOSTIC.md) and
[MOD preflight](docs/MOD_PREFLIGHT.md).

With Git, Python 3, Make and a host C compiler installed:

```sh
make bootstrap
make baseline
make dev
make test
```

The executable, upstream licence, help and build manifest appear in
`build/baseline/`. Licensed AmigaOS/ROM media and local emulator configuration
remain outside Git. `vendor/pt23f/` preserves the pinned upstream snapshot;
the build generates a narrow assembler-spelling compatibility copy.

The user confirmed **ProTracker 2.4G — AmiGUS Edition**, based on native 2.3F,
on 19 September 2026. This replaces the earlier 2.4A product name; the repository
address remains `calfrenetez/Protracker-2.4A` as requested.

## Start here

1. [Processed handover review](docs/HANDOVER_REVIEW.md) — changes, provenance,
   open decisions and current evidence.
2. [Build preparation and acceptance plan](docs/BUILD_PLAN.md) — staged work,
   prerequisites and separate emulator/hardware gates.
3. [Updated supplied scope](docs/handover/2026-09-18-v2/FINAL_SCOPE_2.4G.md) —
   complete product requirements from the latest package.
4. [Supplied package entry point](docs/handover/2026-09-18-v2/README_FIRST.md)
   and [integrity manifest](docs/handover/2026-09-18-v2/MANIFEST.json).

The supplied documents are preserved byte-for-byte as reference material.
Their kickoff text and embedded agent rules do not independently authorize
implementation, deployment, firmware changes or background monitoring. Direct
user instructions control the work. After handover processing, the user
authorized proceeding with the baseline build and verification.

The owner's 19 September instruction authorises independent software development
while the ordered Mini is in transit. Project/pattern persistence and bounded editing cores are implemented. The first
native Enhanced editor, sampler and routing pages are working; enhanced preview,
slice-trigger playback and mixed AmiGUS/MIDI replay integration remain open.
AmiConnect upload/exec and real card tests remain separate acceptance gates.
Initial smoke results do not certify all effects or physical hardware.

Batch track/group WAV export is available through `PT24GRender --stems` or `--groups`; see [reference renderer](docs/REFERENCE_RENDERER.md). The native RENDER WAV panel also provides STEMS (S) and track/group mode (O).

CLI pattern-row excerpts use `--pattern N --from-row FIRST --to-row END` with silent state-preserving pre-roll; the editor MARKED ROWS button (E) applies the current marked rows/tracks to WAV, stems and sample bounce.
