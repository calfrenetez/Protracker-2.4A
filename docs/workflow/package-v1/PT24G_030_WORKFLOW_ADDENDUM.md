# ProTracker 2.4G — native 030 workflow addendum

**ID:** PT24G-030-WORKFLOW-v1  
**Issued:** 3 October 2026  
**Authority:** The user's approval of the three ranked additions in this conversation.  
**Status:** Implementation specification; feasibility remains subject to implementation and measurement on the real target.

## 1. Scope, precedence and non-goals

Implement these additions in the existing native Amiga 2.4G project, in priority order:

| ID | Approved addition | Required outcome |
|---|---|---|
| ZT1 | Sample manager and safe unused-sample cleanup | Inspect, find, audition and edit samples; preview and undo removal of explicitly selected unreferenced material. |
| ZT2 | Loop and selection toolbox | Select the existing loop, jump to either loop boundary, and copy the current selected range to a free slot. |
| ZT3 | Pattern-to-sample/instrument navigation | Select or open the event's relevant resource, with explicit treatment of inherited or ambiguous state. |

These are workflow improvements, not an audio-engine redesign. ZooperTracker provides the reference ideas, including its sample/cleanup screens and the relevant 1.2/1.3 controls. Our precise safety and routing behaviour below is a **2.4G design requirement**, not a claim about ZooperTracker's implementation. See [S1–S3] in `SOURCES.md`.

The existing 2.4G master specification controls the overall architecture. This addendum controls the new interactions and acceptance requirements. Preserve any later explicit user-approved project decisions discovered in the repository; record a genuine conflict rather than silently rewriting the architecture. Do not infer approval from an old assistant suggestion.

Existing undo/redo, sample slicing, sample formats, screen modes and backend facilities must be reused or extended in place. Do not create duplicate managers or a parallel data model. The reconstruction audit remains a separate workstream: use its relevant findings/tests, preserve its inputs, and do not make completion of the entire historical audit a prerequisite for unrelated safe UI work.

**Not added here:** Jam Mode; polyphonic software mixing; automatic BPM, beat or key detection; similarity-based sample merging; automatic transcription; chord-to-arpeggio entry; new WAV/stem/video export modes; a new project or companion-file format; sample-colour redesign; general sample compaction/renumbering; or new recording hardware support. Already approved features are not cancelled by this exclusion list.

## 2. Preserve the real target and existing project

The target is the user's PAL A1200, ACA1234 50 MHz 68030, 128 MB Fast RAM, using the project's established Amiberry and AmiConnect workflow. Measure the actual Chip/Fast memory, OS, display, cache settings and installed drivers; the stated accelerator specification is not a complete runtime configuration.

Keep native 2.3F as the historical foundation at the project's existing pinned revision. Do not replace it with 2.3FC, update upstream automatically or substitute the desktop C/SDL clone. Keep all feature changes in 2.4G development sources or an isolated worktree, not in preserved historical inputs.

Retain the settled track model: up to 16 tracks, one selected destination per track (Paula, AmiGUS or MIDI), and a maximum of four simultaneous Paula channels. Do not conflate a tracker track, an instrument, a sample asset and a hardware voice. Do not silently introduce simultaneous multi-destination routing.

Preserve the existing master-sample/cache design: master data remains authoritative; backend caches are derived and versioned. Do not lower master precision or convert a high-resolution source merely because an editor action or Paula audition was requested. Use the existing conversion/backend policy, with unsupported cases reported clearly. These additions do not establish new AmiGUS format or throughput guarantees.

Do not introduce mandatory FPU instructions, an 060 dependency, SDL, FFmpeg or another desktop runtime. Retain the familiar ProTracker layout and existing approved display modes, including the 640×512 work where implemented. Do not enlarge the display-mode scope. Every new action must be usable with the real keyboard and two-button mouse, without requiring a wheel or touch gestures.

## 3. Preflight and integration deliverable

Before editing, inspect project guidance (including applicable `AGENTS.md`), repository status, branches/worktrees, pinned baseline, current build commands, master requirements and current tests. Preserve uncommitted work. Do not reset, clean, rebase, force-push or overwrite unrelated files to simplify the task.

Create a short integration map containing:

- Existing equivalents for each proposed action, and which pieces actually need implementing.
- Sample/instrument identity and ownership rules; pattern storage; undo transaction API; waveform selection units; playback/preview lifecycle; and current resource-resolution APIs.
- Source files/symbols to change, test commands, supported project/sample formats and maximum sizes.
- Measured or demonstrably available emulator and real-hardware capabilities.
- Genuine dependencies on unfinished 2.4G core work, without labelling stubs as features.

Use the project's established toolchain and licensing rules. Reimplement the selected interaction ideas in native code. Do not assume desktop code/assets can be copied without reviewing their actual licence and compatibility. No third-party application code or assets are supplied in this package.

Continue useful available work when hardware or a backend is missing. Record the blocked acceptance cases and proceed with safe common editor functionality. Do not invent a repository URL, local path, tool command or completed test.

## 4. ZT1 — sample manager and conservative cleanup

### ZT1.1 Sample list and actions

Provide one integrated sample list, reusing an existing screen when practical. Show stable sample ID/slot, name, supported format, length, loop status and usage. Show length in unambiguous units: sample frames and/or decoded bytes, with precision/channel layout stated where relevant. Do not label a word count as bytes or confuse compressed file size with resident audio memory.

Provide compact usage information such as explicit stored reference count and distinct pattern count. Distinguish these from dynamically executed notes: this is not a promise to count every playback occurrence through loops and jumps. For sample assets reached through instruments, show that dependency rather than declaring them unused.

Selection, explicit audition and opening the sample editor must be available. Selecting/highlighting a row must not sound a note. Reuse the existing audition path and its voice policy; do not commandeer a Paula channel or emit MIDI merely to browse the list. If safe audition is not available during song playback, disable it with a reason rather than stealing a song voice.

Sorting/filtering, if provided, affects the view only. IDs, pattern contents, routing and playback order must not change. Stable identity must survive sorting, copying and undo. Do not require an expensive continuously refreshed waveform thumbnail for every row.

### ZT1.2 Define conservative usage correctly

An on-demand scan must cover **all stored patterns and all supported tracks**, including patterns outside the current order list and positions outside current song length that the project still retains. Preserve references in instrument-only events; do not inspect only rows that contain a pitch.

Follow all supported persistent project links: instrument-to-sample maps, defaults, alternate sample assignments, existing slice references and any other registered audio-resource owners. Preserve resources with unresolved or unknown references. A muted track or an unplayed pattern does not make its samples disposable.

Do not treat a zero instrument/sample field as proof of no dependency: apply the project's established state semantics. Conservative preservation is required; complete execution-path analysis is not required to delete obvious unreferenced assets. Mark unknown/ambiguous cases as protected instead of guessing.

Distinguish persistent musical references from transient UI/preview/cache/undo ownership. The latter must be safely released, redirected or retained through transactions; they must not become dangling pointers. Do not assume deleting a slot immediately frees an allocation that an undo record still owns.

A scan produces a revision-tagged preview containing candidate IDs, reasons, protected entries, counts of slots that would become free, and clearly labelled storage/resident-memory estimates. **Unreferenced is not the same as unwanted.** Initially select nothing for deletion; allow an explicit select-all-eligible action. Never run automatic cleanup at load, save, playback start or application exit.

### ZT1.3 Transactional cleanup

Default cleanup removes only the user-selected eligible samples. It does not delete patterns, trim active audio tails, shorten the order list, merge sounds or renumber retained samples. Free slots may remain as gaps. A separate existing validated compaction operation may remain available under its own requirements, but building one is not part of this addendum.

Revalidate the document revision and eligibility before committing. An edit after preview must invalidate or refresh the preview; do not apply a stale plan. Respect protected/reserved slots and all persistent owners. Do not delete external source files or unrelated project assets from disk.

Apply destructive changes only with transport, recording and affected preview activity safely stopped, through the existing engine lifecycle. Ask for an explicit Stop-and-Apply action when necessary; do not silently stop a performance or switch routing. For the initial implementation, it is acceptable to require stopped playback for the full usage scan too; show that requirement before starting it.

One user cleanup must be one undoable transaction. Preflight allocations and undo retention, then commit consistently. Undo must restore sample data, identities, metadata and relevant references; redo must reproduce the deletion. Invalidate derived caches and UI handles through the existing ownership rules.

Report both free slots and actual memory effects honestly. Undo retention can keep sample data resident. Do not advertise the sum of sample lengths as immediately reclaimed Fast/Chip/card memory, and do not silently purge the user's undo history to achieve a savings claim.

When the undo budget or safe ownership transition is unavailable, leave the project unchanged and explain the blocker. Do not quietly make cleanup irreversible. Allocation failures or cancellation before commit must not produce a partially cleaned song.

## 5. ZT2 — loop and selection toolbox

### ZT2.1 Common range model

Provide explicit commands for **Select Loop**, **Go to Loop Start**, **Go to Loop End**, and **Copy Selection to Free Slot**. Retain existing keyboard shortcuts; add nonconflicting bindings after checking the actual keymap. Commands must also be discoverable through labelled controls or an existing menu/page, not modifiers alone.

Define the action boundary using a validated range in sample frames, ideally `[start, end)` with an exclusive end. Translate through an adapter when the legacy editor uses inclusive marks, word units or another convention. Do not rewrite the legacy playback representation just to adopt a convenient UI convention.

Use checked arithmetic for frame offsets, bytes per frame, allocation sizes, alignment and backend/export limits. Support the sample formats already approved and implemented, without silently changing precision, channel count, source rate or tuning. Where the existing format cannot represent an exact range, refuse clearly or request a visibly explained adjustment; never truncate or pad silently.

### ZT2.2 Select and locate the existing loop

Select Loop marks the entire valid enabled loop. It changes the selection, not sample data, playback loop values or playback state. Interpret legacy disabled-loop sentinels through the existing loader/engine rules rather than assuming every short loop is valid.

With no enabled loop, disable the action or show a clear non-destructive status. With corrupt/out-of-range metadata, report it without repairing the source behind the user's back. Empty samples and boundary loops must not crash.

Go to Loop Start/End centres or scrolls the waveform as far as the viewport permits, preserving the current zoom where practical. It does not move the loop brackets or transport. An exclusive end at the sample's final boundary must be displayable without reading a frame beyond the buffer.

### ZT2.3 Copy the actual current selection

Capture the current source identity, source revision, range and format at invocation. Copy the marked audio directly. **Do not use an earlier clipboard range.** Normal clipboard copy/paste remains a separate operation unless the existing UI explicitly documents otherwise.

Choose a genuinely free destination: unreserved, unreferenced, with no sample or instrument resource that would be overwritten. A zero-length but referenced or intentionally named/configured slot is not automatically disposable. Do not trigger cleanup automatically to obtain a slot. With no eligible slot, report that fact and leave the project unchanged.

Copy the selected supported master data exactly, preserving pitch-defining metadata and applicable source volume/tuning parameters. Do not replace the source, rewrite notes to use the new sample, alter track routing or overwrite a populated slot. Selecting the new slot in the editor after a successful copy is allowed.

Loop metadata policy must be deterministic. If the complete valid enabled source loop lies within the copied range, preserve it translated by the selection start. If the loop is absent or only partially included, leave looping disabled in the new sample and make that clear. Never preserve a dangling or accidentally shortened loop. Preserve the source's loop and all source bytes unchanged.

Allocate and prepare the result before publishing it into the slot table. Validate the source revision again before commit. Stop or safely quiesce affected playback/preview state through the existing API; the initial implementation may require stopped transport for the whole copy action. No long interrupt-disabled copy or hardware upload is permitted.

The new sample is independent unless the existing project has a validated copy-on-write implementation. Later editing of either sample must not alter the other accidentally. Treat the operation as one undo/redo transaction and invalidate relevant caches. A failed allocation, user cancellation or exhausted undo budget must leave source, destination and project state unchanged.

### ZT2.4 Waveform performance

Reuse existing waveform summaries where possible, with bounded caches in suitable working memory. Update invalidated regions; do not rescan a multi-megabyte waveform on every pointer movement. Viewing or moving a selection must not upload audio to AmiGUS or rebuild a Paula cache.

For long copies or initial overview generation, use bounded work chunks and progress/cancellation where measurable latency warrants it. Cancellation must be safe before commit. Report real measured latency and peak memory; do not invent a response-time claim or allocate an unbounded duplicate of every sample to make the UI easy.

## 6. ZT3 — navigate from an event to its resource

### ZT3.1 User interaction

Provide two actions: **Select Event Instrument/Sample** and **Open Event Instrument/Sample**. A Ctrl-left-click equivalent may be used when it does not conflict with the established keymap. Provide a menu/button/keyboard alternative, preserve ordinary note entry/block selection, and consume the action so the same click does not fall through to a newly opened screen.

Keep the origin (song/order context where known, pattern, row and track) so the user can return to the same edit location through existing navigation. Selecting/opening must not trigger a note, change transport position, modify effect state, alter route settings or mark musical content dirty.

### ZT3.2 Resolve identity, not just a number

An explicit nonzero event instrument/sample identifier is a direct navigation target, including on an instrument-only row. Label it as the event's instrument. Do not claim it is necessarily the sample currently sounding: portamento and other stateful behaviours can make those concepts different.

For a note/event without an explicit resource identifier, resolve the effective instrument using the **validated existing engine semantics and the actual editing context**. Do not use the sample currently selected elsewhere in the editor, or blindly take the nearest preceding nonzero field in the displayed pattern.

Account for carry across rows/patterns, repeated patterns at different song positions, relevant control flow and instrument-only changes. Reuse an existing resolver, validated state checkpoint or bounded offline replay service where available. Avoid an unbounded path-search or a second subtly different replay engine.

Return a structured result such as `explicit`, `resolved_inherited`, `ambiguous`, `unresolved` or `no_resource`, with origin/context and a reason. A detached pattern or multiple valid entry histories may not have one knowable inherited instrument. In that case report the ambiguity and keep the current target unchanged; never present a guess as correct. Straightforward deterministic inheritance should work, not simply be classified unknown in every case.

Distinguish “instrument selected by this event” from “voice/sample actually sounding at this tick.” Where the engine cannot establish the latter, do not promise it. Do not use an unrelated live playback channel state for an arbitrary event under the editor cursor.

Resolver caches must invalidate when relevant notes, instruments, order/control-flow state or routing change. If precomputation is needed, perform it outside timing-critical playback code and bound its cost.

### ZT3.3 Routing-specific destinations

For Paula/AmiGUS sample-based instruments, open the authoritative sample or the existing instrument editor from which its sample is selected, not a lossy backend cache. For MIDI instruments, open the existing MIDI instrument/routing controls. Navigation alone must not transmit Program Change, Note On or any other MIDI message.

Do not interpret a MIDI program number as a sample slot. An explicit resource can be empty or unresolved; show that accurately rather than navigating to another resource by coincidence. Preserve existing effects, route types and maximum Paula voice count.

## 7. Shared integrity, memory and real-time requirements

Use the project's master/derived ownership rules. Keep general tables, usage maps and waveform summaries in appropriate Fast-memory allocations; keep hardware-visible data and display allocations under the existing Chip-memory/backend policy. Inventory memory by allocation identity to avoid double-counting shared buffers and undo-retained objects.

No allocations, file I/O, full-pattern scans, waveform analysis, blocking locks or lengthy copies in the audio/timer interrupt path. Publish state through existing bounded synchronization; stop/quiesce when a safe concurrent update is not already implemented. Do not disable interrupts for an entire copy or sample deletion.

Read-only navigation, opening the manager and moving a waveform viewport should coexist with playback without adding observable glitches or missed deadlines relative to the pre-feature build. Destructive operations may require stopped playback as specified. Large work must yield to input where supported, have a clear busy state, and avoid a frozen-looking application.

Use bounded undo with actual allocation-failure handling, not unlimited copies based on the advertised 128 MB. Never evade memory pressure by downgrading a master sample, dropping user content, silently purging history or overwriting a slot. Repeated open/copy/undo/redo/cleanup cycles must not leak retained resources after the relevant history is released.

Non-destructive operations must preserve musical data and rendered behaviour. When cleanup removes unreferenced assets, compare decoded musical semantics and playback of retained content; the saved file is expected to change. Existing legacy MOD and native-project import/export limits remain unchanged. Do not silently make a new unsupported song format appear to be an ordinary MOD.

## 8. Implementation order and validation

The ranked priority remains ZT1, ZT2, ZT3. A practical delivery order may put shared primitives first: identity/ownership and undo integration, read-only manager/navigation, loop viewing, copy transaction, then cleanup. Use small reviewable changes, reuse existing functionality and retain regression evidence between steps.

Run applicable tests against the current baseline before changes and the candidate after changes. The test list in `ACCEPTANCE_TESTS.md` is mandatory within supported scope; a not-yet-implemented backend produces a documented blocker, not an invented pass. Preserve synthetic fixture generators and hashes. The supplied JSON is only a planning template.

Use Amiberry first, then the real A1200 through the existing AmiConnect setup when reachable. Verify each capability actually works: deployment, launch, input, completion detection, screenshot/state/audio capture where available, and recovery. An executable starting is not proof that the feature was exercised.

Do not flash FPGA/AmiGUS/accelerator/PLIPbox firmware, change startup/network settings, reconfigure the physical machine or force a reboot to run these tests. Do not invoke physical parallel-port sampling while PLIPbox is attached/in use. None of the three additions requires new live recording tests; use stored sample fixtures.

On hardware, preload fixtures and reduce unrelated transfers/screen polling during playback measurements. Record the installed configurations. Unsupported capture methods remain unsupported; a listening check is useful but not equivalent to a hardware register trace or quantified latency measurement.

## 9. Required deliverables and completion

Deliver native source changes in the existing development workflow; integrated user-facing controls; short usage/keymap notes; a requirement-to-code/test map; regression fixtures and runners; and a concise implementation report with exact build/artifact hashes, commands, test environments, memory/performance observations and blockers.

Each feature must have an implementation status and a separate validation status. Distinguish `IMPLEMENTED`, `PARTIAL`, and `BLOCKED` from `EMULATOR_VALIDATED`, `REAL_A1200_VALIDATED`, and `NOT_VALIDATED`. A disabled button, mock, source review or emulator-only run cannot establish real-hardware completion.

The addendum is complete only when the three approved workflows operate within the supported 2.4G scope, required regression tests pass, undo/data ownership is safe, and their critical workflows and playback-coexistence checks have passed on the real 030 target. When hardware access is missing, deliver useful implementation and emulator evidence with the remaining hardware gate explicitly open.

Do not claim universal compatibility or extrapolate unmeasured throughput. The final report should state exactly what was implemented, which formats/routes/limits were exercised, what the measurements show, and what remains unresolved.
