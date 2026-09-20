# ProTracker 2.4G — AmiGUS Edition

Build preparation for a native Amiga tracker with 1–16 channels, classic
ProTracker workflow, and exclusive Paula, AmiGUS or MIDI routing per channel.

**Status: native 2.4G dev2 builds and runs in the isolated Amiberry test setup.**
This increment integrates classic MOD preflight and input.device mouse support.
The portable channel, PCM/WAV, extended-project, transactional document, pattern
undo and slicing cores pass host and native Amiga tests. PT24GConvert now supports
verified new-file project saves and strict lossless classic MOD round trips.
A separate native Enhanced editor prototype now connects project loading, note
entry, four-column paging, pattern undo and verified saving. Its first replay
path adds classic four-channel Paula song/pattern playback, sample audition and
live pattern edits; [mixed-backend playback remains open](docs/PAULA_REPLAY.md).
Native Load/Save New dialogs now support the workflow from a blank song through
load, edit, play, save and reopen, with cancelled/invalid loads preserving edits. The
Enhanced screen now follows the owner's full classic reference after the
earlier simplified layout was rejected. Incremental redraws preserve identical
pixels while reducing six-cursor drawing time by about 95% in the private emulator;
native mouse controls and keyboard/file workflows pass. EDIT OP. now adds marked
block copy/paste/clear/transpose, whole-pattern copying and atomic undo.
CLEAR / Control-N creates blank songs with 1–16 channels through a staged,
non-destructive failure path. SAVE MOD exports eligible projects losslessly through a native dialog; enhanced
projects are refused without dropping their data. CHANNEL / Control-R now exposes
exclusive routing plus mute/solo, with chronological note/channel undo and exact
project persistence. Classic Paula playback applies mute/solo live; mixed
AmiGUS/MIDI replay is still pending. SAMPLER / Control-L now adds waveform range
selection, exact integer PCM WAV/IFF/explicit RAW import/export and undoable reverse, normalize,
gain, fades and DC removal. LOOPS/SLICES tabs add undoable forward/pingpong
metadata, baked crossfade and editable manual/AUTO SLICE proposals; marker changes
cannot silently retarget existing slice-note ordinals. RANGE provides zoom/pan and
exact frame entry; FORMAT adds explicit whole-sample precision/rate conversion with
scaled metadata and atomic undo. The default integer filter suppresses downsampling aliases and supports native
progress/Escape cancellation; linear mode remains an explicit faster option. High-resolution samples remain editable/saveable;
classic Paula audition still requires compatible samples. See the
[editor guide](docs/ENHANCED_EDITOR.md) and retained native UI evidence. Physical AmiGUS tests await the owner's Mini.
MIDI note ownership and quantised recording into undoable pattern events now
pass host/native sink tests; CAMD and live transport remain unconnected.
See the [MIDI/recording design](docs/MIDI_RECORDING_CORE.md) and [development checkpoint](docs/HARDWARE_INDEPENDENT.md),
[baseline report](docs/BASELINE_BUILD.md), [diagnostic evidence](docs/AMIGUS_DIAGNOSTIC.md)
and [MOD preflight](docs/MOD_PREFLIGHT.md).

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
