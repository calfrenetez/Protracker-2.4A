# Acceptance tests — PT24G 030 workflow addendum

**This is a test specification, not a test report. Every row starts `NOT_RUN`.**

Read the main addendum first. Test requirements below do not grant permission to alter the user's production music, historic reference binaries, firmware, boot settings or network setup.

## 1. Fixtures and independent expectations

Generate small deterministic fixtures in the existing repository's test conventions. Do not depend on copyrighted commercial songs. Store the generator, input seed where relevant, decoded expected event list, supported format and hashes.

At minimum include: a simple legacy song; an out-of-order pattern reference; instrument-only and inherited-instrument rows; a repeated pattern with two distinct entry contexts; a muted high-numbered track; a sample-based instrument map; an unreferenced named sample; a protected/reserved slot; a full-slot project; distinct wave regions A/B for the clipboard regression; full/partial/end-boundary loops; empty resources; and current supported higher-precision master samples. Add a MIDI fixture only through the actual existing MIDI model.

Use deliberately distinguishable sample data (for example separate marker sequences), and compare the resulting byte/frame ranges independently of the function under test. Large-sample and size-boundary cases must use the actual supported limits found in the code, with those limits stated in results. Never assume every planned 2.4G format/backend already exists.

Malformed input and fault-injection tests belong in disposable emulator environments. Use safe, representative subsets on the real machine. Neither crash fuzzing nor physical recording is needed to demonstrate these additions.

## 2. Environments and evidence

Record build/source hash, test fixture hashes, emulator/OS/ROM identifiers as applicable, CPU/cache/memory configuration, screen mode, audio route/driver/firmware identity, tool versions and observed run completion. Record ROM hashes locally without distributing ROM images.

Use equivalent initial project, clipboard, selection, transport and undo state for comparison runs. Repeat core read-only playback tests to expose nondeterminism. Do not accept an unchanged screenshot as proof that an intended action ran. Inject a deliberately different expected output in a test-only comparator check so the comparison's failure path is known to work.

For exact sample edits, compare bytes plus relevant metadata. For cleanup, distinguish changed file bytes from preserved musical content. For timing/audio, identify the capture method and its limitations. Analogue recordings need justified alignment/tolerance; do not require identical WAV bytes from physical analogue capture. Listening notes are qualitative evidence, not a register trace.

Use `PASS`, `FAIL`, `BLOCKED`, `NOT_RUN`, `INCONCLUSIVE` and, only for a documented version/format distinction, `NOT_APPLICABLE`. A missing peripheral, unimplemented route or unavailable real machine is `BLOCKED`, not `NOT_APPLICABLE`. Preserve a reason and the pending verification for every non-pass. Pre-existing failures must stay visible, separately attributed.

## 3. Required test cases

The environment column is a target requirement, not a claim that a test has run. Detailed fixtures and scripts are to be created/reused by the receiving development chat.

### Shared / baseline

**BASE-01 — Clean pre-feature and candidate builds**  
Action: Record native source revisions, build commands and executable hashes; launch/load/play/stop/quit with the same fixture.  
Required observation: Both builds behave as observed; any pre-existing failure is recorded separately.  
Environment: Emulator + real A1200.

**BASE-02 — Read-only operation sequence**  
Action: Open/close manager, change selection/view and navigate among samples during playback.  
Required observation: No musical-data changes, unsolicited notes, route changes or observable playback regression.  
Environment: Emulator + real A1200.

**BASE-03 — Failure and undo-budget injection**  
Action: Reject allocations/undo reservations at each transaction preparation point.  
Required observation: No partial deletion/copy, lost data, stale visible state or dangling ownership.  
Environment: Emulator fault injection; benign low-memory case on hardware.

**BASE-04 — UI action/keymap check**  
Action: Use real keyboard/two-button mouse equivalents in every supported display mode.  
Required observation: All actions discoverable; old bindings retained; no click-through or unintended note entry.  
Environment: Emulator + real A1200.

### ZT1 / sample manager

**SM-01 — Small known reference graph**  
Action: List samples and inspect explicit references, pattern counts, format/size/loop labels and instrument dependencies.  
Required observation: Counts match independently enumerated stored references; units and identities are correct.  
Environment: Emulator + real A1200.

**SM-02 — Unordered patterns and instrument-only events**  
Action: Preview cleanup on references outside the song order and on instrument-only rows, including higher tracks.  
Required observation: Every retained reference protects its resource; scanning is not limited to four tracks or audible rows.  
Environment: Emulator + real A1200.

**SM-03 — Persistent indirect/protected resources**  
Action: Include instrument maps, defaults, existing slices, reserved slots and an unknown reference type.  
Required observation: Known dependencies are retained; unknown/protected entries cannot be deleted as unused.  
Environment: Emulator; available project structures on hardware.

**SM-04 — View sort/filter and audition**  
Action: Sort/filter if supported, select a row, explicitly audition, then open editor.  
Required observation: Sorting changes no ID/notes; selection is silent; audition uses existing safe route/voice policy.  
Environment: Emulator + real A1200.

**SM-05 — Stale cleanup preview**  
Action: Edit a reference or add a dependency after preview, then request apply.  
Required observation: Preview is invalidated/recomputed; a newly referenced sample is not deleted.  
Environment: Emulator + real A1200.

**SM-06 — Preview/cancel/apply/undo/redo**  
Action: Select a subset of truly unreferenced samples; cancel once, then apply, undo and redo.  
Required observation: Cancel is unchanged; apply affects only selected IDs; undo/redo restores/removes exact data and metadata.  
Environment: Emulator + real A1200.

**SM-07 — Transport/preview active**  
Action: Request destructive cleanup while song, recording state or sample preview is active.  
Required observation: No silent interruption or unsafe mutation; Stop-and-Apply or clearly disabled action; no retained voice accesses freed memory.  
Environment: Emulator + real A1200, no physical recording required.

**SM-08 — Undo-retained memory**  
Action: Observe free slots and allocation counts after deletion, undo, redo and later release of history.  
Required observation: Report distinguishes logical deletion from actual resident reclamation; no double-counting or silent history purge.  
Environment: Emulator + real A1200.

**SM-09 — Clipboard/current editor/cache owners**  
Action: Delete eligible material while transient editor/clipboard/cache state refers to it, then navigate/paste/undo as supported.  
Required observation: Transient state is safely retained/invalidated by documented policy; no dangling reference or wrong replacement asset.  
Environment: Emulator; safe workflow on hardware.

**SM-10 — Save/reload cleaned song**  
Action: Save cleaned project and supported legacy representation to test-only paths; reload and play retained material.  
Required observation: Only intended unreferenced content is absent; retained IDs/references/timing remain valid; source files untouched.  
Environment: Emulator + real A1200.

### ZT2 / loop and selection

**LP-01 — Valid loop including final boundary**  
Action: Select loop and visit both boundaries at several zoom levels.  
Required observation: Selection is exact; zoom/view changes do not alter loop/data; final exclusive endpoint never reads beyond buffer.  
Environment: Emulator + real A1200.

**LP-02 — No loop, empty sample and invalid metadata**  
Action: Invoke each loop action with absent/disabled loop; malformed cases in disposable emulator.  
Required observation: Clear non-destructive status; no invented loop, automatic repair or crash.  
Environment: Emulator; valid empty/no-loop cases on hardware.

**LP-03 — Stale clipboard adversary**  
Action: Copy region A to clipboard; mark distinct region B; invoke Copy Selection to Free Slot.  
Required observation: Destination contains B, not A; source and ordinary clipboard policy remain correct.  
Environment: Emulator + real A1200.

**LP-04 — Selection boundaries and units**  
Action: Copy first/last frames, full sample and declared size boundaries in every supported master representation.  
Required observation: Exact requested representable range, checked lengths and preserved format/tuning; unrepresentable ranges refused/explained.  
Environment: Emulator + representative formats on real A1200.

**LP-05 — Contained and partial source loop**  
Action: Copy a region containing the full loop, then one containing only part of it.  
Required observation: Full loop translated correctly; partial loop disabled in new sample; source loop and bytes unchanged.  
Environment: Emulator + real A1200.

**LP-06 — No genuinely free slot**  
Action: Populate/reserve/reference every destination, including a zero-length referenced or named slot.  
Required observation: Clear failure without overwrite, auto-cleanup, renumbering or source mutation.  
Environment: Emulator + real A1200.

**LP-07 — Copy/edit independence and undo**  
Action: Create sample, edit destination and source independently, then undo/redo each operation.  
Required observation: No unintended aliasing; full sample/metadata restored; backend caches reflect each generation.  
Environment: Emulator + real A1200.

**LP-08 — Revision race, cancellation and partial allocation**  
Action: Change source revision during preparation or cancel/fail a long copy before commit.  
Required observation: Operation aborts/restarts safely; no partial slot publication or invalid retained allocation.  
Environment: Emulator; user cancellation on real A1200.

**LP-09 — Backend cache invalidation**  
Action: Copy/edit/undo with each currently supported sample route, then audition explicitly.  
Required observation: Audition reflects current master generation; masters are not silently downgraded to cache format.  
Environment: Emulator + available Paula/AmiGUS hardware.

### ZT3 / navigation

**NV-01 — Explicit and instrument-only event**  
Action: Navigate from explicit sample/instrument rows, including one with no pitch.  
Required observation: Correct resource selected/opened without note playback or musical edits.  
Environment: Emulator + real A1200.

**NV-02 — Deterministic inherited instrument**  
Action: Resolve zero-field events after instrument-only rows and across a known pattern/order boundary.  
Required observation: Expected inherited target from independently specified state history, not current editor selection.  
Environment: Emulator + real A1200.

**NV-03 — Repeated/detached pattern ambiguity**  
Action: Use the same pattern after two different entry histories; also view it without a song context.  
Required observation: Correct target with unique known context; explicit ambiguity/unresolved status when context cannot identify one.  
Environment: Emulator + real A1200.

**NV-04 — Portamento and sounding-voice distinction**  
Action: Use an event where explicit instrument selection and currently sounding sample need not match.  
Required observation: Navigation identifies the labelled event target; never claims unsupported exact sounding-voice resolution.  
Environment: Emulator + real A1200.

**NV-05 — MIDI resource destination**  
Action: Navigate explicit/inherited MIDI events and an empty/unknown instrument.  
Required observation: Opens correct existing controls or clear unresolved state; no sample-slot confusion and no transmitted MIDI message.  
Environment: Emulator + real A1200/MIDI monitor where available.

**NV-06 — Resolver cache and route edits**  
Action: Resolve, edit relevant order/instrument/route data, and resolve again; use a row not currently playing.  
Required observation: No stale result or misuse of unrelated live channel state; no track routing changed by navigation.  
Environment: Emulator + real A1200.

**NV-07 — Origin/return and event propagation**  
Action: Open target during playback and return to origin using keyboard/mouse controls.  
Required observation: Original edit location retained; transport unaffected; no click-through, dirty musical data or unsolicited audition.  
Environment: Emulator + real A1200.

### Performance and integration

**PERF-01 — 030 interaction measurements**  
Action: Measure cold/warm manager open, navigation, loop jumps and long-copy/scan duration under recorded workloads.  
Required observation: Report actual latency/work size/peak memory; no unsupported response claims; no prolonged unresponsive UI.  
Environment: Real A1200 mandatory; emulator supplementary.

**PERF-02 — Chip/Fast low-memory behaviour**  
Action: Record allocations and safe failures near supported memory limits, including largest tested samples.  
Required observation: No unbounded cache/undo growth, overflow, silent format downgrade or lost content.  
Environment: Emulator stress + benign real A1200 case.

**PERF-03 — Repeated edit/cleanup cycles**  
Action: Repeat defined copy/undo/redo/cleanup sequences, release history normally and compare retained allocations.  
Required observation: No unaccounted growth after documented caches settle; all surviving resource owners valid.  
Environment: Emulator + real A1200.

**PERF-04 — Playback coexistence**  
Action: Compare identical fixture playback before/after feature changes while performing read-only UI actions.  
Required observation: No new observable glitches or missed timing; evidence strength and capture limits explicitly recorded.  
Environment: Emulator + real A1200 mandatory.

**INT-01 — Supported format/route matrix**  
Action: Exercise core workflows across current native/legacy formats and supported Paula/AmiGUS/MIDI destinations.  
Required observation: Unsupported/unfinished backend cases remain BLOCKED, not hidden or treated as passes; all available routes retain semantics.  
Environment: Emulator + real A1200/available peripherals.

**INT-02 — Pristine evidence and artifact review**  
Action: Review diff, build artifact, keymap notes, fixture/result paths and untouched historical inputs.  
Required observation: Changes isolated to intended 2.4G work; historical baseline and unrelated files unmodified.  
Environment: Host review.

## 4. Performance acceptance and reporting

Do not invent cycle/latency budgets without measuring the existing application. Establish a baseline and state the workload for each observation: pattern/track/reference count, sample format/size, display mode, undo budget and enabled backend.

Measure cold and warm UI operations, long scan/copy duration and peak/retained memory. Read-only operations must not create new observable playback glitches or missed deadlines. Mutating operations may require stopped transport. Long-running non-real-time actions need a meaningful busy/progress state and safe cancellation where implemented. A particular progress mechanism is not mandatory when an action is demonstrably instantaneous at the tested maximum.

For every performance limitation, retain the measurement and the fallback (for example stopped-only scan or bounded waveform cache), rather than claiming that 128 MB Fast RAM guarantees arbitrary workloads. Identify the heaviest actually tested case and keep untested maxima explicit.

## 5. Completion checklist

- All three workflows operate in the existing native 2.4G UI; no duplicate subsystem or unrelated feature expansion.
- Reference graph, source ranges, loop translation, contextual instrument resolution and ownership transitions have tests.
- Cleanup/copy failure, cancellation, undo/redo and low-memory behaviour are safe.
- Legacy/native save/reload and supported route behaviour remain correct within documented scope.
- Real 030 interaction, memory and playback-coexistence evidence is retained.
- Unavailable routes/peripherals, incomplete core features and missing hardware remain explicit open gates.
- Historical source/audit inputs and user music are untouched; builds, usage notes and reports have exact artifact paths.

## 6. Run record schema

`TEST_MATRIX.json` contains planning rows only. Keep actual results in a separate run record using the existing test framework, with at least:

```json
{
  "test_id": "LP-03",
  "status": "NOT_RUN",
  "build_sha256": null,
  "fixture_sha256": null,
  "environment_id": null,
  "started_at": null,
  "commands_or_actions": [],
  "observations": [],
  "artifact_paths": [],
  "reason_or_limitation": "Template only; no test execution is claimed."
}
```

A populated planning matrix is not a completed run, and a feature cannot be called real-A1200 validated until its applicable critical cases have actual hardware observations.
