# Build preparation and acceptance plan

This plan organises the updated supplied scope. The reproducible baseline build
and initial emulator smoke checks are complete; the broader compatibility and
hardware gates remain pending. See [BASELINE_BUILD.md](BASELINE_BUILD.md) for
current evidence. Product naming is tracked in [HANDOVER_REVIEW.md](HANDOVER_REVIEW.md).

## Current priority while the Mini is in transit

On 19 September the owner requested all development that does not depend on
physical AmiGUS tests. Independent software portions of stages 1, 3–5 and 7–10
can therefore proceed before the physical gate in stage 2. Hardware evidence is
still required before those backends are accepted. The native loader/input and
portable channel/PCM/WAV checkpoint is documented in
[HARDWARE_INDEPENDENT.md](HARDWARE_INDEPENDENT.md); its scope and missing UI/replay
integration are explicit. No duplicate diagnostic profile is being created.

## First build milestone

1. Pin native [2.3F source](https://github.com/8bitbubsy/pt23f) by full commit and
   source date. Preserve upstream licence/credits and inventory binary resources.
2. Read the pinned build instructions and select an assembler that actually
   accepts the source dialect. Record exact assembler/linker versions and a clean
   build command. Do not assume a generic 68k toolchain is compatible.
3. Reproduce the unmodified baseline; save logs, inputs and executable SHA-256.
   Explain differences from an upstream binary if byte identity is unavailable.
4. Locate the existing AmiBerry environment and record emulator, OS/ROM and
   configuration identifiers. Verify launch/exit, normal MOD load/play/edit/save,
   reopened output and repeated relaunch using disposable legal fixtures.
5. Locate the existing AmiConnect real-A1200 interface. Verify bounded upload,
   execution, exit/result collection and recovery using a harmless diagnostic
   before tracker/hardware changes. Do not infer protocol commands from prose.
6. Audit 2.3FC where source/licence permits and classify each candidate change.
   Establish effect/timing traces and loader/edit/lifecycle hardening fixtures;
   freeze a known-good clean baseline after its acceptance checks.

Current source and public SDK pins are in `baseline.lock.json` and
`amigus-sdk.lock.json`. The first diagnostic's evidence and the developing
AmiConnect upload/exec dependency are recorded in
[AMIGUS_DIAGNOSTIC.md](AMIGUS_DIAGNOSTIC.md).

## Staged implementation

| Stage | Scope | Exit evidence |
| --- | --- | --- |
| 0 | Reproducible native 2.3F, 2.3FC audit, compatibility/hardening and both automation tiers | Build manifest, retained executable, emulator regression logs, verified real-machine automation interface and clean baseline. |
| 1 | Sequencer/effects, exclusive routing, backend/input boundaries, sample/project/event model and shared converter architecture | Reviewed interfaces; route invariants; deterministic global-effect ordering; format design covers OFF, slices, capabilities and future migrations. |
| 2 | Scriptable AmiGUSTest with timeout/result handling and safe ownership/cleanup | Real hardware detect/reserve/free, interrupt install/remove, sample upload and one then 4/8/16 voices; pitch, pan, loops and interpolation evidence. Track Mini and Zorro separately. |
| 3 | Stable 1–16 channel hardware tracker, P/A/M route state, paging, navigation, mute/solo, Classic/Enhanced screens | Four-column workflow; fifth Paula assignment rejected; all playback/audition/stop paths covered; applicable effect parity; measured display/UI budgets. MIDI unavailable until its backend is delivered. |
| 4 | Extended project persistence, high-resolution sample model, PP20 and RAW/IFF/8SVX/WAV | Golden round trips, bounded parsers, safe failure without replacing current work, explicit precision conversion and genuine classic MOD round trips. |
| 5 | Undo/redo, recovery, Recent Projects, sample operations, manual/AUTO slicing | Reversible edits and unchanged slice masters; save-failure/recovery tests; successful-path-only recent list and graceful missing paths. |
| 6 | Direct AmiGUS capture | Variant-specific measured formats/rates; 24/48 precision where supported; safe interruption, overrun reporting and usable sampler output. |
| 7 | CAMD output/input, endpoint selection, explicit OFF, quantised recording and Hold Record | Note ownership/release, meaningful effect translation, clock/control behaviour, endpoint loss/reconnect and real external-MIDI checks. |
| 8 | AmiConnect/Safari application input/control | Physical/injected dispatcher parity; clicks, release, drag and semantic commands; no stuck button/record state after disconnect. |
| 9 | Rendering/stems/bounce; in-app classic export and shared standalone Amiga converter | Deterministic pinned traces/output; true 24-bit master path; clear MIDI omissions; all four conversion classifications; generated MODs verified in actual 2.3F. Later Mac UI shares the core. |
| 10 | Seven-note chord editor, VU, selected MOD sample import and workflow additions | Classic-looking operation, safe packed-MOD import and measured low-end metering cost. |
| 11 | Separate Studio PCM transport/mixer | End-to-end low-eight-bit precision evidence, physical FIFO/underrun tests and measured sustainable voice count with responsive UI on the 030/50. |
| 12 | Optimiser, duration, polish and release qualification | Full regression, roughly 60-minute 16-channel physical endurance, lifecycle/resource stability, human audio/UI/MIDI checks and compatibility report. |

The full [supplied scope](handover/2026-09-18-v2/FINAL_SCOPE_2.4G.md) remains the
requirements reference; grouping requirements here does not remove them. Design
converter/reusable rendering interfaces early even though their completed user
flows come later. Hardware-voice delivery does not depend on finishing Studio.

## Test evidence contract

Every run should record build/source/test-suite IDs, environment tier and exact
configuration, case ID, result, duration, counters, logs and relevant output
hashes. Use PASS, FAIL, BLOCKED or NOT RUN; never turn an unavailable facility into
a passing test.

- **Host checks:** format/conversion/unit/property checks where portable code
  exists, fixture hashes and static integrity. These do not prove Amiga execution.
- **AmiBerry:** launch/exit, editor/data/file compatibility, malformed inputs,
  safe saves/recovery, deterministic effects and non-hardware input/MIDI logic.
- **Physical Amiga:** actual card/library ownership, bus/interrupt behaviour,
  sound/capture, 1/4/8/16 hardware voices, PCM, timing, display/UI contention,
  endurance, resource release and real remote/MIDI interfaces.
- **Human review:** audible artifacts/timing feel, classic visual/UI fidelity,
  capture quality and external-MIDI behaviour at meaningful milestones.

Maintain an immutable legal golden corpus plus disposable malformed/synthetic
fixtures. Test failed/truncated loads, allocation failure, extreme loop/sample
bounds, disk-full/write-error/interrupted saves and repeated acquire/release.
Record CPU, Chip/Fast RAM and executable-size changes against the clean baseline.

## Compatibility and release records

Keep a machine-readable manifest and readable report containing exact 2.3F and
audited FC revisions, compiler/assembler/linker, OS/ROM, machine/CPU/memory,
Mini/Zorro variant, FPGA/core, library/API/driver, CAMD, AmiConnect, AmiBerry,
project-format version, tests and remaining limitations.

Check official AmiGUS releases at project start, relevant hardware milestones
and release candidates. Record installed/known-good combinations and retest
approved changes. Do not create a recurring monitor or perform firmware updates
as a consequence of processing the handover.

The package excludes combined routes, variable pattern lengths, multiple effect
columns, 32+ channel UI, groove/micro-delay/aliases/Pattern Matrix, chance/Nth or
Euclidean features, scale-aware entry, multisample instruments, plugins/large
software synths, wholesale OctaMED/PT3/DAW redesigns and unrelated brainstormed
features. Keep these exclusions visible during design reviews.
