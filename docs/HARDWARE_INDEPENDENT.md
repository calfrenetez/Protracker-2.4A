# Hardware-independent development checkpoint — 19 September 2026

The owner has ordered an AmiGUS Mini, expected in approximately five days, and
explicitly requested continued development of everything that can be built and
checked without the card. This changes the implementation order in the supplied
handover: physical hardware acceptance remains required, but is not a gate on
independent editor, format, processing and input work.

## Native tracker increment

`make dev` builds **ProTracker 2.4G dev2** from the locked native 2.3F source.
The vendor snapshot is unchanged; narrowly anchored edits apply to a generated
copy and fail if the expected baseline changes. The known-good 2.3F target is
still independently reproducible.

The classic MOD load path now performs a bounded preflight before clearing the
current song. Recognised `M.K.`/`M!K!` files are checked for song length, every order,
pattern count, truncated patterns/samples and invalid sample-number high bits.
It uses 2,108 bytes of workspace rather than copying all sample data. The actual
loader also checks read lengths and its allocated pattern capacity.

The in-tracker compatibility policy accepts M.K. files with up to 100 patterns,
matching the inherited extended load option; the separate strict PTModCheck
continues to distinguish standard 64-pattern M.K. from 100-pattern `M!K!`.

This is **not a complete transactional loader**. Preflight failures preserve the
current work, as verified by subsequent save comparison. Allocation failures,
files changed after preflight and late I/O failures do not yet have full rollback.
Unknown signatures and PP20/PX20 retain their legacy dispatch paths; those paths,
15-sample modules and Load Song still need their own hardening and regression
coverage. Passing legacy dispatch is not a validity claim for such files.

The input boundary now tracks RAWMOUSE left/right transitions and uses that
shared state at all 112 original button-polling sites. It consumes absolute
NEWPOINTERPOS/PIXEL events addressed to its own screen, clamps coordinates and
clears fractional movement. Button releases are processed even while disk
movement is suppressed, and focus loss clears both button states. Relative
RAWMOUSE retains the original movement behaviour. Unrelated CIA/filter accesses
remain unchanged. TABLET/NEWTABLET, relative PIXEL events and pointers addressed
to other screens continue through the OS chain; they are not claimed as supported
tracker pointer paths. The retained integration probe covers the left button;
right-button modifiers, drag and focus/disconnect scenarios need broader UI
regression before input compatibility is accepted.

The native guest injector opens input.device and submits pointer/button events
with IND_WRITEEVENT. Its click opened Disk Op while CIA's physical left-button bit
remained released (64) before, during and after the injected press. This verifies
the tracker input path, not the complete browser/AmiConnect transport. AmiConnect
must separately recognise this derivative's capture layout. The tracker owns a
320x12 one-bit Intuition shell and draws its 320x256 display via private copper
and bitplanes; generic screen dimensions do not describe its visible surface.

Program/status version strings are 2.4G. The inherited bitmap logo still says
2.3F and awaits a matching classic-style asset update. The native editor and
replayer still use their original four-channel song representation.

## Reusable software cores

The following C99 components run on both the host and the Amiga. They do not
allocate internally, have caller-supplied bounded storage and use no FPU.
They are **not yet connected to the native tracker UI/replay path**.

| Component | Implemented and tested | Integration still required |
| --- | --- | --- |
| Channels | 1–16 channels, four-column page selection, wrapping navigation, names/pan/group/MIDI metadata, exclusive P/A/M routes, atomic fifth-Paula rejection and stable allocation of four Paula slots | Expanded pattern/event model, editor pages, backend dispatch, live audition and replay |
| Mute/solo and history | Availability-aware audibility without route substitution; bounded undo/redo snapshots for channel metadata, redo invalidation and oldest-snapshot eviction | Pattern/sample undo, UI commands, persistent recovery |
| PCM editing | Signed 8/16/24-bit mono/stereo samples; reverse, saturation gain, shared-peak normalize, fades and per-channel DC removal; validation before mutation | Sampler UI, loop/slice metadata, selection and undo wiring |
| Conversion | Explicit precision conversion with rounding/clipping; offline integer linear resampling | Antialias filtering and quality qualification; shared renderer/converter integration |
| WAV | Bounded RIFF PCM parsing and canonical writing; 8/16/24-bit mono/stereo, odd-chunk padding, byte/block-rate checks and full low-eight-bit retention | Float/extensible WAV, loop metadata, sample-load UI, file I/O transaction and large-file policies |

The linear resampler does **not** yet provide an antialias filter for high-quality
downsampling. The 24-bit path preserves stored PCM precision; it does not prove
real-time Studio performance, hardware output precision or an achievable voice
count. There is no claim of sixteen software-mixed voices on the 030.

## Verification and reproduction

```sh
make bootstrap
make baseline
make dev
make test
make core-mutations
AMIGA_CC=/path/to/complete/m68k-amigaos-gcc make core-tests
```

Nineteen Python test groups exercise baseline integrity, host sanitized channel
and PCM/WAV cases, ownership failure handling, MOD bounds, exact emulator profile
guarding and native source preparation. The core mutation run passed 100,000
iterations under AddressSanitizer/UndefinedBehaviorSanitizer, including malformed
WAV parsing, route changes and PCM edit sequences. Its deterministic seed is
0x24a. The earlier MOD preflight mutation run covers another 200,000 cases and is
documented separately in MOD_PREFLIGHT.md.

The native core binaries target `-m68000 -msoft-float`. A complete GCC 6.5/nix20
toolchain supplied the required 64-bit integer division helpers. The previously
used local GCC 13.4 installation had an empty libgcc archive and could not link
these operations; that other installation was not changed. Compiler, archive,
source and executable hashes are recorded in core-build.json. This establishes
native execution under the configured emulated 68030 without FPU, not physical
68000 performance. The tracker keeps its baseline assembler flags, including
`-m68020` for the original guarded 32-bit instructions.

Native evidence covers channel and PCM/WAV suites, 15 cases using the exact
assembly preflight with real DOS reads/seeks, OS-only injected Disk Op selection,
ordinary mouse/keyboard operation, classic fixture playback and rejected-load
save preservation. Screenshots, logs and hashes are under
`evidence/hardware-independent/`. Dev1 and dev2 are kept separate so earlier
evidence is not silently attributed to a changed executable.

Amiberry access was explicitly handed off by the AmiConnect task. Only the exact
`Config=ProTracker isolated baseline` instance, private disk copy and disposable
PTDEV files were used. Licensed OS/ROM files are excluded from the repository and
development package. No physical Amiga, AmiGUS library ownership or firmware was
changed by this work.

## Continuing software work

Versioned projects, extended patterns, persistence and bounded pattern undo are
now implemented as shared cores. Next is their native editor integration. Effects/replay traces, undo/recovery, save-failure
handling, renderer/converter, MIDI logic and Enhanced screen work remain open.
The full supplied scope still applies; this checkpoint does not complete any
stage whose UI, replay or hardware acceptance remains missing.

When the Mini arrives, run the bounded diagnostic first and record its driver,
firmware and machine configuration. Positive detect/reserve/free, interrupt
lifecycle, sample transfer, 1/4/8/16 voices, audible quality, PCM/capture and
endurance remain **NOT RUN on hardware**. No user decision is needed to continue
the independent software work.

## Project, editing and converter increment

The version 1.0 [project format](PROJECT_FORMAT.md) stores 1–16 channels, raw MOD
periods or MIDI notes, explicit OFF, one effect column, 8/16/24-bit mono/stereo PCM,
loop/slice metadata and MIDI endpoint settings. CRC, required capabilities,
versioned chunks and preserved optional extensions make unsupported input explicit.
Staged document loads preserve the old project and dirty flag through corruption,
capacity rejection and allocation failure. The strict MOD importer preserves an
optional original header so an eligible MOD can export byte-for-byte unchanged.

Pattern edits use a bounded command/change journal with undo/redo, dirty/save
revisions, conflict detection, block copy/paste, pattern clone and checked
transpose. Slicing provides non-destructive transient proposals, manual markers
and an explicit offline crossfade operation. None of these new core operations
is yet integrated with native tracker playback or its sampler interface.

`make converter` builds the host utility; `make core-tests` also builds native
`PT24GConvert`. Commands are `inspect INPUT`, `project INPUT NEW_OUTPUT` and
`mod INPUT NEW_OUTPUT`. MOD analysis identifies the required conversion strategy;
only lossless direct export is implemented. Conversion, bounce, MIDI audio capture,
normal overwrite saves and recovery are still open. Existing destinations are
refused. Output is staged, closed and read back before new-file publication.
Native DOS uses an exclusively created staging directory and Rename; the native
file test verifies that an existing destination is preserved. This does not
establish power-loss durability or a safe replacement policy.

Evidence in `evidence/project-v1/` records 19 host test groups, 5,000 project parser
mutations under ASan/UBSan (including CRC-repaired semantic mutations), and 14
native test/CLI cases. The native cases cover pattern undo, slices, channel/PCM
regression, file safety, project round trips, document allocation/save faults,
converter output, refusal to replace existing files and refusal of lossy export.
The classic fixture round trip is byte-identical. Golden project fixtures are in
`tests/fixtures/project-v1/`. These are software/emulator results, not card tests.

The native allocation-failure test exposed incorrect structure-field addressing
from this local GCC 6.5 toolchain's extra late optimizer. A retained minimal
reproducer and before/after assembly are in the same evidence directory. Native
C builds now pass `-fbbb=-` when that option is available. All 14 cases passed with
that mitigation; compiler/source/executable hashes and flags are retained. This
is a targeted mitigation, not a certification of the compiler. The unchanged
assembler build does not use that compiler pass.


## Enhanced editor integration

The separate `PT24GEdit` development executable now connects extended project
loading, note/instrument/effect entry, OFF, page navigation, bounded pattern
undo/redo and verified new-file saving to a native 640x512 Intuition interface.
It uses the original bitmap font and the supplied four-column visual direction.
[ENHANCED_EDITOR.md](ENHANCED_EDITOR.md) describes its commands and limitations.
Playback is disabled; the original assembler tracker/replayer remains separate.

`evidence/enhanced-editor/dev1/` retains 20 host groups, the rerun 14-case native
core/converter suite after sharing the save adapter, and actual editor UI runs.
Native keyboard edits produced exactly the independently specified saved bytes
and CRC. Undo/redo, dirty-state retention after overwrite refusal, byte-identical
reopen/resave and two normal exits were checked. The optimized host renderer was
pixel-identical to the initial renderer. Completed-frame waits replaced an early
screenshot taken before the first redraw. Human visual/UI acceptance, native
mouse interaction and physical display-performance checks are still pending.


## MIDI ownership and recording increment

[MIDI_RECORDING_CORE.md](MIDI_RECORDING_CORE.md) records the monophonic-per-track
output policy, shared-note co-ownership, stable endpoint identities, reconnect
cleanup and recording collision policy. The recorder implements Hold Record,
whole-row quantisation, velocity and explicit OFF. Its pattern adapter maps
through the song order list and uses the existing undo journal while preserving
current effects and refusing to overwrite existing notes. These cores remain
unconnected to the native editor controls, CAMD and the sequencer clock.

`evidence/midi-recording/dev1/` retains 22 passing host groups and the expanded
16-case native suite, including MIDI sink/ownership and recording/undo tests.
No external MIDI messages or physical audio were used. The editor executable is
byte-identical to the one with retained Enhanced UI evidence. These additions
therefore do not imply external MIDI, recording feel or replay acceptance.

## Enhanced Paula replay integration, 19 September 2026

The Enhanced editor now has a first owned four-channel Paula/CIA backend, using
the pinned 2.3F standalone replay effect code. Song playback, pattern looping,
sample audition, future-row live edits, Stop/F00 resource cleanup, playback row,
current tempo and voice sample waveforms are connected. Unsupported mixed-route
or enhanced sample projects are refused explicitly. See [replay details](PAULA_REPLAY.md).

The native replay harness verified note periods/volumes, busy-owner refusal,
DMA shutdown, immutable source data, repeated restarts, live edits, audition,
speed/tempo/volume/cut effects, F00 and CIA exhaustion without removing another
owner's vector. Alternate CIA-A timer-B execution remains NOT RUN on this
Workbench profile because the OS already owns it; source-level removal checks
pass. The real editor input test played a MOD, changed C-2 to D-2, played the
changed pattern, saved, reopened, played D-2 and resaved byte-identically, with
four DMA channels enabled during playback and zero after Stop.

This supersedes the earlier "playback disabled" prototype status. It does not
certify every 2.3F effect combination, physical sound quality, phase-accurate
quadrascopes, full mixed backend dispatch or physical UI performance. Full-frame
redraws are still slow in the test profile; CIA replay continues independently.

## Native file requesters and renderer, 19 September 2026

The Enhanced editor can now start with a blank song, load a MOD/PTG with LOAD or
Control-O, and select a new save path with DISK OP. > SAVE NEW or Control-Shift-S.
Control-S uses a supplied command-line destination or opens the requester. Dirty
loads need a second Load action; other input cancels that confirmation. Cancelled
or invalid loads preserve the current document/history. Existing destinations are
still refused, and successful saves still use staging/readback verification.

The native workflow test verified blank start, dialog load, an independently
checked D-2 note and CRC, new save, overwrite refusal, dirty confirmation/cancel,
invalid load, loading the saved file, playing its changed note and byte-identical
resaving to another name. The renderer optimization produced identical pixels
and cut measured three-frame draw time from 224 to 151 50-Hz ticks in the same
emulator configuration. Full-frame refresh remains a performance limitation.

Replay also now gives empty samples and initial instrument-zero notes an owned
Chip RAM silence word and a nonzero DMA length, and rejects out-of-range one-word
sample-loop metadata. Native checks exercise these boundary cases alongside the
previous playback/ownership/effect tests. Vendor replay code is still untouched;
the generated ABI/setup adapter supplies the safe initial state.

## Incremental display and native mouse controls

The current Enhanced editor updates affected rows/status regions for ordinary
editing and retains full redraws for page, scroll, panel and dialog changes.
Host tests compare complete images and simulated dirty-rectangle copies against
the full renderer. Six cursor redraws measured 14 PAL ticks versus 311 for full
frames in the private emulator, with matching hashes. Full frames remain costly;
physical responsiveness and 50 Hz display acceptance remain untested.

`evidence/enhanced-editor/dev5/` retains matching executable hashes for native
file-dialog, mouse and keyboard/history/save/reopen tests. Mouse tests use OS
input events to open/back out of Disk Op, play/stop and audition/stop, with the
physical CIA button bit released throughout. This does not test the complete
AmiConnect browser transport. All four DMA channels were off after Stop and the
editor exited normally. Amiberry ownership was released after guarded shutdown.

## Native block editing

EDIT OP. now provides marking across rows/channel pages, copy/paste, clear,
semitone transpose, select-all and unmark. Copy freezes the selection. Paste is
atomic and refuses pattern-edge overflow; transpose rejects the whole selection
if any note has an unsupported raw period or leaves its pitch range. Effects,
instruments, velocity and slice fields survive transpose. Whole-pattern copy into
an explicitly selected existing pattern provides cloning. Each modifying action
uses one pattern-journal command, including all 1,024 events of a 16-channel pattern.
The heap editor now occupies 76,354 bytes on 68k, including bounded clipboard and
scratch space; no 24 KiB temporary pattern buffer is placed on the Amiga stack.

Host checks cover reverse selection, cross-page copy, frozen selection, edge and
pitch refusal, full-pattern undo/redo, clipboard reset and byte-identical partial
rendering. The native UI run copied/transposed a block into another channel page,
cleared and undid/redid it, refused edge overflow, cloned a whole pattern, and
saved exactly the independently specified event bytes and CRC. Reload/resave was
byte-identical; a reopened document had an empty clipboard. A separate classic
MOD was transposed through the same controls, replayed at period 404 and stopped
with all DMA channels off. All three editor instances exited normally.
