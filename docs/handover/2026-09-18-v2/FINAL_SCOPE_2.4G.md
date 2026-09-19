# ProTracker 2.4G — AmiGUS Edition
## Definitive Codex Scope

This is the single authoritative scope. Do not add brainstormed features not listed here.

## Product and compatibility
- Base the work on native Amiga ProTracker 2.3F 68k source.
- Preserve the classic ProTracker 2.x appearance, colours, typography, hexadecimal workflow, keyboard/mouse usability and readable four-channel pattern layout.
- Advanced features belong on additional ProTracker-style pages/panels; do not redesign it as a modern DAW or adopt the PT3 high-resolution UI.
- A normal 2.3F MOD must load without conversion, play/edit normally, save as a genuine MOD when still compatible, and round-trip back to 2.3F.
- Preserve a classic Paula replay/reference path and regression-test the normal ProTracker effect set, timing, samples, loops, finetune and editing.

## 16-channel model
- Support 1–16 tracker channels.
- Display four at once: 01–04, 05–08, 09–12, 13–16.
- Fast page switching and continuous Tab/Shift-Tab navigation.
- Architect for possible future 32-voice AmiGUS use, but 2.4G is capped at 16.

## Definitive per-channel routing
Every tracker channel selects exactly ONE destination:
**PAULA | AMIGUS | MIDI**
- No combined-output modes.
- Any of channels 1–16 may use Paula, but no more than four may be Paula-routed simultaneously.
- Dynamically map those routes to Paula's four physical voices.
- Reject a fifth Paula assignment clearly; never silently steal one.
- AmiGUS routes use local AmiGUS hardware audio.
- MIDI routes sequence an external MIDI destination and do not also produce local sample audio.
- A normal four-channel MOD defaults channels 1–4 to Paula.

## AmiGUS hardware
Support both full-size Zorro II AmiGUS and PCMCIA AmiGUS mini.
Use `amigus.library` as the common abstraction wherever practical and isolate variant/bus details.
Support 68000-class hardware-voice operation where practical, including compatible A500+Zorro/AmiGUS configurations.
Primary enhanced test machine: A1200 + ACA1234 68030/50 + 128 MB Fast RAM + AmiGUS mini; no FPU dependency.
Query available RAM rather than assuming 128 MB is free.

## AmiGUS hardware-voice mode
- Direct AmiGUS wavetable playback, not AHI.
- 8/16-bit source samples; 16-bit up to 48 kHz is the main enhanced target.
- Use hardware start/stop/loop, pitch/phase, interpolation, volume and variable stereo panning.
- AmiGUS performs voice playback/mixing rather than CPU-mixing the 16 hardware voices.
- Preserve applicable ProTracker effects.

## Variable panning
- Independent full-left-to-full-right panning for enhanced AmiGUS channels.
- Store enhanced panning in the 2.4G format.
- Classic MOD imports initially reproduce traditional Paula placement.
- Never silently lose enhanced panning on export.

## 24-bit Studio mode
Separate genuine 24-bit/48 kHz direct AmiGUS PCM-streaming mode.
- Preserve true 24-bit source data.
- No silent 16-bit downgrade.
- Fixed-point 68030 optimisation; no FPU requirement.
- Benchmark sustainable voice count on the real 68030/50 while the UI remains responsive.
- Do not claim 16 simultaneous Studio voices unless hardware testing proves it.

## Extended project format
Versioned 2.4G format must preserve: 1–16 channels, PAULA/AMIGUS/MIDI routing, 8/16/24-bit samples, rates, panning, AmiGUS state, MIDI settings, track names/groups, slices, forward/ping-pong/crossfade loops and other enhanced state.
Keep standard MOD import/export where representable. Warn and require explicit choice before lossy export.

## Direct AmiGUS sampling
- Record directly through AmiGUS inputs.
- Capability-detect Zorro versus mini differences.
- 8/16/24-bit-aware sample model, rate metadata, mono/stereo where supported.
- 24-bit/48 kHz capture is a key A1200 workflow.
- Captures enter the normal sampler.
- 24->16 conversion for wavetable use must be explicit.

## Sample editor
Keep the ProTracker sampler look/workflow and add:
Normalise; Amplify; Fade In/Out; Reverse; DC-offset removal; Resample; explicit bit-depth conversion; crossfade-loop creation; forward loops; ping-pong loops; zero-crossing-aware editing where useful.

## Sample slicing
- Non-destructive multiple slice markers on one underlying sample.
- Trigger slices from normal pattern notes using a defined mapping.
- Use AmiGUS start/stop positions where suitable.
- Preserve slice metadata and allow MIDI triggering where configured.
- ProTracker-style slice UI.

## Automatic transient slicing
Add AUTO SLICE:
- offline 68k-friendly transient/attack detection
- sensitivity control and sensible minimum spacing
- optional refinement toward suitable zero crossings
- proposed markers remain fully editable
- never destructively chop source audio

## Undo/redo
Undo/redo pattern edits, sample edits, metadata and other destructive operations where practical.
Use Fast RAM when available and dynamically scale history for smaller machines.

## Crash-recovery autosave
- Configurable recovery snapshots of changed unsaved work.
- Offer recovery after crash/reset when a valid newer snapshot exists.
- Avoid repeated floppy writes; allow disable/configuration.
- Use safe/atomic replacement where practical.
- Never overwrite the user's original song as the recovery mechanism.

## 16-channel workflow
Track names; mute/solo; optional logical groups for organisation/stems; pattern clone/duplicate; improved block/selection operations; song optimiser; song duration display; continuous page navigation.
Do NOT add variable pattern lengths.

## MIDI
Use CAMD.
MIDI-routed channels support channel 1–16, Note On/Off, velocity, Program Change, relevant controllers/pitch bend, optional Clock and Start/Stop/Continue.
Architect for MIDI In.

## Quantised live recording
- Record MIDI/input into normal editable patterns.
- Configurable quantisation to the ProTracker row/grid model.
- Use CAMD timestamps where appropriate.
- Deterministic and lightweight enough for classic Amiga hardware.
- Integrate with Hold Record.
- Do not add Renoise micro-delay/groove/aliases/Pattern Matrix/synced-sampling/variable-pattern systems or Cubase-derived features.

## AmiConnect/Safari remote control
Make 2.4G controllable through the existing AmiConnect Safari/browser environment and solve the current browser-click incompatibility.
Inspect the actual input path and AmiConnect mechanism first.
Physical and injected input converge on one internal dispatcher.
Support pointer move, left/right down/up, click, useful double-click, hold and drag (including sliders/waveform selections), plus semantic commands where AmiConnect permits them (Play Song/Pattern, Stop, Record, select position/instrument, mute/solo, load/save/render).
Do not blindly fake CIA hardware if a cleaner application-level route exists.

## Rendering/export
Add Song->WAV, Pattern->WAV, selected range where practical, selected track(s)->WAV, stem export, selected sample->WAV and WAV import.
Support sensible 16/24-bit stereo 44.1/48 kHz output; 24-bit/48 kHz Studio master rendering is a key workflow.

## Bounce/render to sample
Current pattern, selected channels and selected range where practical -> new assignable sample.
Allow 24-bit/48 kHz enhanced bounce; conversion to 16-bit wavetable use is explicit.

## Legacy sample formats
RAW load/save where appropriate; IFF/8SVX load/save where representable; WAV load/save; preserve loop metadata where supported; explicit warnings/conversion for incompatible formats.

## PowerPacker/PP20
Directly load legacy PowerPacker-compressed MODs.
Detect from content, safely decrunch before MOD validation, bounds-check output, preserve current state on corrupt input, test genuine packed MODs, investigate historical `powerpacker.library`, and verify licence compatibility before embedding decompression code.
Optional classic-MOD PowerPacker saving may be retained where practical/legal; enhanced projects stay in the 2.4G format.

## Selected PT3.x-era additions
- Seven-note chord editor: ProTracker-style, audition locally where applicable, render/bounce chord to sample.
- Hold Record: optionally wait for first playable keyboard/MIDI event; integrate with quantised recording.
- Genuine VU meters across the 16-channel architecture without expensive unnecessary 68000 metering.
- Import a selected sample/instrument from another normal or safely decrunched MOD without replacing the current song.
- Good AmigaOS coexistence: avoid unnecessary busy waits/resource monopolisation and coexist safely with AmiGUS, CAMD, AmiConnect and normal tasks.
Do NOT import PT3 high-resolution UI, DYN, Maestro-specific paths unless independently required, or wholesale PT3 redesigns.

## Automated development/testing
Two test tiers are available and MUST be established early.

### Tier 1 — AmiBerry on the Mac
Automate build/smoke/regression tests for launch/exit, MOD load/play/edit/save, 16-channel model/UI, project round trips, PP20, RAW/IFF/WAV, undo, sample processing, slicing/auto-slicing, rendering/bounce and non-hardware-specific MIDI/input logic.
AmiBerry passing is not proof of AmiGUS hardware correctness.

### Tier 2 — AmiConnect + real A1200
The existing setup can automatically upload/run/test code on the real A1200 and retrieve results. Inspect/use its actual interface.
Test AmiGUS detection/reservation/interrupts, sample upload, 4/8/16 voices, panning, pitch, loops, interpolation, PCM, 24/48 Studio, underruns, recording, endurance, UI responsiveness, injected Safari input and cleanup/relaunch.

## AmiGUSTest diagnostic
Before deep tracker integration, build a small scriptable diagnostic with machine-readable PASS/FAIL/error output covering conceptually:
DETECT/INFO, RESERVE/FREE, interrupt install/remove, sample upload, play/stop, 4/8/16 voices, pan, pitch, loop, interpolation, PCM, 24/48 PCM, recording, stress and clean shutdown.
Choose actual command syntax only after inspecting the automation environment.

## Endurance/performance
Automate real-A1200 tests including ~60-minute 16-channel stress playback, repeated load/play/stop/relaunch, panning/loop/interpolation stress, UI activity during playback, repeated resource acquire/release, recording and progressive Studio voice-count tests.
Track measurable underruns/timing errors/crashes/leaks.
Determine safe Studio capacity empirically.

## Automation safety
Timeout hangs; preserve known-good recovery build; never auto-flash AmiGUS FPGA during ordinary tests; avoid destructive disk operations; isolate disposable test data; release resources on normal error paths; distinguish emulator from hardware results.

## Human milestone validation
After automated tests, obtain human checks at meaningful milestones for audible artifacts, timing feel, UI feel, classic visual fidelity, recording quality and real MIDI behaviour.

## Explicit exclusions
NOT in 2.4G:
- combined output routes such as Paula+MIDI or AmiGUS+MIDI
- Renoise micro-delay, groove/swing, aliases/Pattern Matrix/Song Map, synced sampling or variable pattern lengths
- modern Cubase-derived features discussed during brainstorming
- deterministic chance/Nth triggering
- Euclidean fill
- scale-aware entry
- XM-style multisample instrument system
- arbitrary 32/64/128-channel UI
- multiple effect columns
- VST/plugin architecture
- large software synth subsystem
- wholesale OctaMED command emulation
- modern DAW UI redesign
- PT3 high-resolution UI/DYN as AmiGUS features

## Implementation order
1. Reproduce unmodified PT2.3F build.
2. Establish 2.3F compatibility regression baseline.
3. Establish AmiBerry automation.
4. Establish AmiConnect real-A1200 deploy/run/log loop.
5. Define clean audio/input/project/file-loader abstractions.
6. Build AmiGUSTest.
7. Prove one, then 4, 8 and 16 AmiGUS voices on hardware.
8. Implement stable 1–16 channel AmiGUS hardware mode + panning.
9. Implement classic-looking channel paging/navigation/routing/mute/solo.
10. Extended 2.4G format/high-resolution sample model.
11. PP20 and legacy RAW/IFF/WAV compatibility.
12. Undo/redo + recovery autosave.
13. Enhanced sample editor + manual slicing + AUTO SLICE.
14. Direct AmiGUS sampling.
15. MIDI Out + per-channel MIDI routing.
16. Quantised live recording + Hold Record.
17. AmiConnect/Safari input/control integration.
18. Rendering/stems/bounce.
19. PT3-derived chord/VU/import/workflow additions.
20. 24-bit/48 kHz Studio engine and empirical capacity tests.
21. Optimiser/duration/polish.
22. Full regression/endurance/human validation.

Parallel work is allowed when dependencies permit. Do not let Studio software mixing block delivery of the stable 16-channel hardware tracker.



# FINAL ADDITIONS AGREED AFTER THE PREVIOUS DEFINITIVE SCOPE

The requirements below are authoritative and form part of the definitive 2.4G scope.

## Screen modes and visual reference

2.4G must look like **classic ProTracker 2.x given more room**, not like a
modern tracker/DAW redesigned for the Amiga.

Provide two presentation modes:

### CLASSIC — 320x256
- closely preserve ProTracker 2.3F proportions and visual character
- primary compatibility mode for lower-end/68000 systems
- enhanced functionality is reached through ProTracker-style secondary pages

### ENHANCED — 640x512
- primary enhanced workspace for the user's A1200 + ACA1234
- retain four comfortably readable tracker columns, not 8 or 16 squeezed columns
- use the extra vertical resolution mainly for more visible pattern rows
- use extra space for breathing room and only restrained permanent information
- retain chunky classic bitmap typography, grey/black panels and classic controls
- do not copy the ProTracker 3.x 640x256 interface
- use a modest Amiga palette (roughly 16/32 colours as technically appropriate)
  and benchmark AGA/Chip-RAM bandwidth rather than assuming display cost is free

`VISUAL_REFERENCE_640x512.png` is the preferred conceptual reference for the
Enhanced screen. It is not a pixel-perfect implementation spec. Preserve its
uncluttered ProTracker-2.x character. Do NOT use the later dense mock-up with
large permanent routing/MIDI/mixer side panels as the design reference.

## Permanent route identification

In enhanced mixed-route projects, every visible tracker channel must be easily
identified by output destination:

- **P** = Paula
- **A** = AmiGUS
- **M** = MIDI

The letter is the primary identifier. Optional user-configurable route colours
may reinforce it but must not be the only cue.

## Optional pattern colouring

Provide optional pattern-display colouring while preserving a fully classic mode:

- CLASSIC
- INSTRUMENT (2.3FC-inspired sample/instrument colouring if adopted)
- EFFECT
- INSTRUMENT+EFFECT, only if legible

EFFECT colouring should be semantic rather than assigning 16 arbitrary colours.
Suggested families:
- pitch: 0/1/2/3/5
- modulation: 4/6/7
- volume: A/C
- sample: 9
- song/pattern/timing: B/D/F
- E subcommands classified by their actual semantic family where practical

Effect colours are derived from pattern data and need not modify a MOD.

## Effect behaviour across PAULA / AMIGUS / MIDI

Implement one authoritative ProTracker effect/sequencer engine and translate its
musical state through the selected output backend.

- Paula and AmiGUS should achieve musical equivalence for applicable voice effects.
- MIDI should translate meaningful effects to MIDI operations, e.g. pitch
  modulation/slides to Pitch Bend, note cut to Note Off, and appropriate
  volume changes to MIDI volume/expression handling.
- Effects with no meaningful MIDI equivalent, such as sample offset, must be
  explicitly N/A/ignored for MIDI rather than given arbitrary behaviour.
- Global sequencer effects such as speed/tempo, position jump, pattern break,
  pattern loop/delay affect the whole song regardless of the route of the
  channel containing the command.
- Paula, AmiGUS and MIDI must therefore remain synchronised.
- UI/help may indicate effect applicability by backend.

## 2.3FC audit

Before major 2.4G changes, obtain and source-diff 2.3FC against the exact pinned
2.3F baseline where source/licence access permits.

Classify FC-only changes and selectively incorporate worthwhile:
- bug fixes
- robustness improvements
- loader improvements, including large/100+ pattern handling
- useful build/tooling improvements

Do not automatically adopt 2.3FC instrument colouring; it is optional under the
pattern-colouring requirement above. Verify provenance/licensing before copying
code.

## 2.3F bug discovery and hardening

Do not assume current 2.3F is bug-free. Make hardening an early milestone.

Pin the exact upstream 2.3F commit/date, reproduce it, and turn known historical
bug classes into regression tests where applicable, including raw-sample
loading, A500/A500+/A600-specific behaviour, large/128K sampling, cut/copy
allocation and quadrascope regressions.

Audit:
- memory bounds and integer overflow
- sample/pattern/order/file-size calculations
- failure-path cleanup
- malformed/truncated file handling
- sample loop/edit endpoints
- Workbench/CLI startup, shutdown and repeated relaunch
- input-state edge cases
- timing assumptions across 68000/020/030/040/060
- library, interrupt and hardware-resource ownership

Add AmiBerry fuzz/property tests using malformed MOD/PP20/IFF/WAV inputs,
extreme sample/loop values and random safe sample-edit operation sequences.
Acceptance: no Guru/crash, out-of-bounds write, project-state corruption or
unsafe resource leak.

Do not "fix" authentic ProTracker compatibility quirks merely because the code
looks strange. Classify a finding as genuine bug, compatibility quirk, or
enhanced-mode limitation.

Freeze a tested `2.4G-clean-baseline` before large AmiGUS refactoring.

## Final routing clarification

The previously specified routing model remains definitive:

**each of the 16 tracker channels selects exactly ONE of PAULA, AMIGUS or MIDI.**

There are no PAULA+MIDI, AMIGUS+MIDI or other combined routes in 2.4G.
No more than four channels may be assigned to Paula simultaneously, but those
four may be any of the 16 tracker channels.



# FUTURE-PROOFING & COMPATIBILITY REQUIREMENTS

These requirements are part of the authoritative development scope.

## AmiGUS release/core/driver compatibility watch

AmiGUS firmware/core and software compatibility must be tracked deliberately.

At project start, before major AmiGUS milestones, and before release candidates:
- check the official AmiGUS project/release sources for newer FPGA core/firmware,
  `amigus.library`, driver, header/API and software releases
- record the installed/known-good versions used for real-hardware tests
- compare new release notes/source/API/register definitions against the pinned
  known-good combination
- flag changes affecting wavetable voices, registers, sample memory, PCM,
  interrupts, recording, timing, library ABI/API or card detection
- run the appropriate AmiGUSTest + tracker regression suite against a newly
  approved software/core combination before marking it supported
- maintain a compatibility matrix separately for full-size Zorro AmiGUS and
  AmiGUS mini

Do NOT query for releases on every local build. Build/test reproducibility is
more important. Checks occur at explicit compatibility checkpoints.

Never automatically flash FPGA/core firmware. If compatibility testing requires
a newer core, stop and request explicit human approval before any persistent
firmware update. Preserve the previous known-good environment/recovery route.

## Dependency/version manifest

For every release candidate, generate a machine-readable and human-readable
build/compatibility manifest containing, where applicable:
- ProTracker 2.4G commit/build ID
- pinned 2.3F upstream commit/date and audited 2.3FC source/version
- assembler/compiler/linker/tool versions
- AmigaOS/ROM version used in tests
- CPU/model/memory configuration
- AmiGUS variant
- FPGA/core/firmware version
- `amigus.library` and relevant driver/API versions
- CAMD version/driver used
- AmiConnect version/protocol capability used for remote tests
- AmiBerry version/configuration
- project-format version
- test-suite version/results

This must make an old successful build reproducible later.

## Capability detection, not version guessing

Where possible, detect required hardware/software capabilities rather than
assuming them solely from a version number. Unsupported/new capabilities should
fail gracefully with a clear message rather than crash or write unknown registers.

Keep AmiGUS hardware access behind a narrow backend/API boundary so new card/core
revisions can be adapted without rewriting the tracker/effect engine.

## Stable internal interfaces

Maintain explicit internal boundaries for:
- tracker/sequencer/effect engine
- audio routing
- Paula backend
- AmiGUS backend
- MIDI/CAMD backend
- sample/project representation
- file import/export
- UI/input dispatcher
- AmiConnect remote-control adapter
- rendering/bounce
- diagnostics/testing

Avoid exposing raw hardware/register assumptions throughout unrelated editor code.

## Extended-format forward/backward compatibility

The 2.4G project format must:
- carry an explicit format version
- use bounded, validated chunks/sections or equivalent extensible structures
- allow unknown optional data to be skipped safely
- reject unsupported required features clearly
- preserve old project loading as the format evolves
- never reinterpret old fields silently
- document migrations
- include corruption/bounds validation
- have golden-file round-trip fixtures for every released format version

Do not tie project compatibility to executable version strings alone.

## Feature flags/capability metadata

Enhanced projects should record only the capabilities they actually require
(e.g. 16 channels, 24-bit samples, MIDI routing, slicing), allowing 2.4G to
explain why a project cannot be fully represented/exported on a target rather
than simply declaring the entire file incompatible.

## Graceful degradation

If AmiGUS, CAMD, AmiConnect or another optional facility is absent:
- the program should still launch where its base requirements are met
- unaffected Classic/Paula/editor features should remain usable
- unavailable functions are disabled/explained cleanly
- no optional dependency should cause an uncontrolled startup failure

## Golden compatibility corpus

Maintain a version-controlled corpus of representative legal test assets:
- ordinary 2.3F MODs
- effect/timing edge-case MODs
- large-pattern MODs
- PP20-packed MODs
- RAW/IFF/8SVX/WAV samples
- 2.4G projects for each released project-format version
- malformed/corrupt fixtures for safe-failure tests

Record hashes for immutable golden fixtures. Use generated/synthetic fixtures
where redistribution rights are uncertain.

## Deterministic/offline reference rendering

For regression purposes, provide a deterministic reference-render path where
practical. Known songs/patterns should produce stable expected state/event traces
and/or output hashes under a pinned test configuration. Hardware-output listening
remains necessary, but deterministic reference tests make subtle replayer changes
detectable automatically.

## Save safety and project integrity

For normal project saves and recovery:
- prefer write-new + validate + atomic/rename replacement where AmigaOS/filesystem permits
- never destroy the last known-good project/recovery file on a failed save
- include explicit dirty-state handling
- test disk-full/write-error/interrupted-save behavior
- validate a newly written enhanced project before considering save complete where practical

## Resource-leak and lifecycle testing

Add repeated automated open/close/reload/restart tests for:
- libraries
- AmiGUS ownership/interrupts
- CAMD resources
- screens/windows
- files
- memory allocations
- remote-control resources

A long-running editor session and hundreds of load/play/stop cycles must not
steadily consume memory or leave hardware resources owned.

## CPU and memory budgets

Establish measurable budgets for:
- 68000 Classic mode
- 68000 AmiGUS hardware-voice mode
- A1200 68030/50 enhanced mode
- 640x512 Enhanced UI
- Studio mode

Track executable size, Chip RAM, Fast RAM and timing/CPU regressions across
milestones. New convenience features must not accidentally make Classic mode
unusable on its intended lower-end systems.

## Release compatibility report

Every release candidate should produce a concise compatibility report listing:
- configurations actually tested
- configurations expected but not physically tested
- AmiGUS core/library combinations tested
- known limitations
- project-format compatibility
- Classic MOD regression status
- Studio-mode measured limits
- outstanding hardware-specific issues

Never present emulator-only validation as equivalent to physical-hardware validation.



# CLASSIC PROTRACKER 2.3F DOWN-CONVERSION / EXPORT — IN SCOPE

2.4G must be architected from the outset to support conversion of enhanced
2.4G projects into the closest practical ProTracker 2.3F-compatible MOD.

This is a down-converter/compiler, not a promise of lossless conversion for
features that classic MOD cannot represent.

## Shared portable conversion core

Implement the conversion logic as a portable shared core (C where practical)
rather than duplicating algorithms in unrelated applications.

The same conversion engine should be usable by:
1. ProTracker 2.4G itself on Amiga via **DISK OP -> EXPORT -> PT2.3F MOD**
2. a standalone native Amiga utility (working name `PT24GConvert`)
3. a later macOS front end/utility for faster batch conversion and richer reports

The native Amiga conversion/export path is a first-class requirement. Do not
make conversion dependent on a Mac. The primary A1200 + ACA1234 68030/50 +
128 MB Fast RAM is capable of offline conversion; expensive conversions may
simply take longer than realtime.

The macOS front end is a convenience/batch-processing target and should use the
same conversion core so equivalent settings produce equivalent results.

## Conversion analysis/report

Before conversion, analyse the project and report:
- source channel count and required 4-channel reduction
- sample count
- 16/24-bit samples requiring 8-bit conversion
- resampling requirements
- slice expansion requirements
- panning that cannot survive classic MOD
- enhanced loop types requiring conversion
- enhanced effects/metadata requiring approximation or removal
- MIDI-routed channels that cannot become audio without captured/rendered external audio
- estimated/identified bounce requirements
- any 31-instrument or classic-format limit conflicts

Offer explicit user control rather than silently discarding information.

## Conversion strategies

Support at least:
- direct/lossless mapping where the project is already classic-compatible
- high-quality offline 16/24-bit -> 8-bit conversion, with appropriate dithering where useful
- resampling to a classic-ProTracker-compatible representation
- conversion of slices to individual samples where limits permit
- conversion/approximation of enhanced loops where meaningful
- mapping arbitrary panning to classic Paula channel placement
- automatic four-voice allocation for sparse material where safe
- bounce/render of selected groups/parts into samples when necessary to reduce dense 5–16 channel material to four Paula voices

Do not claim an exact conversion where information has been transformed.

## MIDI limitation

MIDI events alone do not contain the sound of an external synthesizer.
For MIDI-routed tracks:
- permit exclusion from the classic MOD conversion, or
- use separately captured/rendered audio supplied by the user/workflow
- never invent the external instrument's sound from MIDI data alone

## Result classification

Classify conversion outcome clearly, e.g.:
- **LOSSLESS** — directly representable in 2.3F MOD
- **CONVERTED** — sample/data transformations required
- **BOUNCED** — multichannel material rendered into classic-compatible samples
- **INCOMPLETE** — some material, commonly uncaptured external MIDI audio, could not be represented

## Output integrity

Conversion always creates a new MOD and never destructively replaces the source
2.4G project unless the user explicitly chooses a separate normal save action.

Generated output must pass strict classic MOD validation.

## Automated compatibility verification

Use the existing automation to verify generated MODs:
2.4G project -> converter -> generated MOD -> actual ProTracker 2.3F under
AmiBerry and, at selected milestones, the real Amiga -> load/play/regression.

Maintain golden conversion fixtures for lossless, converted, bounced and
expected-incomplete cases.

Where deterministic rendering is used, compare event traces/output hashes under
pinned configurations as appropriate.

## Architecture requirement

Keep the enhanced project parser, effect semantics, sample conversion and
rendering primitives reusable between ProTracker 2.4G and the converter.
Do not create a second incompatible interpretation of the 2.4G format.

Designing for this exporter is required during the project-format/audio-engine
architecture phase even if the polished standalone/macOS front ends are
delivered after the core 2.4G tracker.



# OCTAMED 8/8.1-INSPIRED FINAL ADDITIONS — IN SCOPE

The following three additions are agreed scope. They must preserve the classic
ProTracker visual/workflow philosophy.

## Recent Projects

Add a ProTracker-style **DISK OP -> RECENT** facility.
- remember approximately the last 10 successfully opened/saved songs/projects
- support standard MOD, PP20-packed MOD and enhanced 2.4G project paths
- newest successful item first
- do not add corrupt/failed opens to the recent list
- missing/moved files must fail gracefully and may be marked/removed
- avoid cluttering the main tracker screen
- store the list in an appropriate preferences/config location
- allow clearing the list
- do not make startup dependent on every recent path being accessible

## Explicit MIDI Note-Off event

The enhanced 2.4G pattern/event model must support an explicit **OFF** note event
for MIDI-routed channels.

Requirements:
- `OFF` is musical event data, not an arbitrary repurposing of a classic effect command
- on a MIDI-routed tracker channel, OFF sends the appropriate MIDI Note Off for
  the note(s) owned by that tracker channel according to the MIDI voice policy
- manual pattern entry must support OFF
- MIDI live recording must record key-release/Note-Off information as OFF events
  at the appropriate quantised position
- display OFF clearly in the normal note field while preserving ProTracker-style columns
- OFF semantics must be saved in the extended 2.4G format
- classic MOD export must explicitly report that MIDI OFF events are not directly
  representable unless their musical result has been otherwise converted/bounced
- do not change the semantics of classic ProTracker `ECx` note cut; it remains a
  separate effect with backend-specific translation where applicable

Design the event representation so future non-MIDI uses of an explicit release
event can be considered without changing the file format, but do not invent
additional scope now.

## CAMD MIDI endpoint selection

Make MIDI input/output device/cluster selection explicit.

Provide a classic ProTracker-style MIDI setup page with at least:
- selectable CAMD MIDI input/source
- selectable CAMD MIDI output/destination
- clear indication when a configured endpoint is unavailable
- safe fallback/no-MIDI behavior when CAMD or the endpoint is absent
- persisted preferences where appropriate

If CAMD exposes multiple useful output destinations, architect per-track MIDI
routing so a MIDI-routed tracker channel can identify the intended CAMD
destination as well as MIDI channel, without changing the definitive rule that
the track's output type is simply MIDI.

Do not hard-wire the application to one serial MIDI interface. Enumerate/use
CAMD facilities and degrade gracefully.

Add endpoint discovery/change/reconnect cases to MIDI regression tests.
