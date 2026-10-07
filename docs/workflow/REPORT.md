# Native 030 workflow implementation and validation

The approved three additions are integrated into the current project, using its
existing editor, authoritative masters, bounded allocator and chronological undo
journal. No baseline/vendor update, new audio engine or deferred ZooperTracker
feature is included. The original sixteen dirty display/editor/harness paths are
preserved separately and excluded from the feature commit. Their actual working
layout is also checked separately from the clean committed candidate.

| Addition | Implementation | Native validation |
|---|---|---|
| ZT1 sample manager and selected cleanup | IMPLEMENTED: stable silent list, conservative stored-reference scan, initially unselected preview, explicit Stop+Apply, atomic undo with retained ownership | NOT_VALIDATED: Amiberry and real A1200 exercises pending |
| ZT2 loop/range toolbox | IMPLEMENTED: half-open loop navigation, exact independent current-range copy to proven free slot, translated contained loops/markers, bounded cancellable overview | NOT_VALIDATED: Amiberry and real A1200 exercises and long-sample latency/memory pending |
| ZT3 event/resource navigation | IMPLEMENTED: silent explicit/inherited identity, bounded real flow/pitch traversal, ambiguous/unresolved states, MIDI routing destination, captured edit return | NOT_VALIDATED: Amiberry and real A1200 input/routing/playback coexistence pending |

## Validation scope

Seven existing sanitizer groups passed before editing. Final host regressions,
working-layout observations, exact 68k candidates and their source/build hashes
are recorded below when their final checks finish. A host controller or planar
render exercises the common code; it does not establish native input, GUI speed,
IRQ timing, musical output, physical device or human listening acceptance.

The native RAM-only controller fixture covers silent selection/navigation,
stopped/capture gates, 24-bit stereo current-range copy, 16-bit mono cleanup,
project save/reload and undo. Its Fast-pool/type/budget assertions will only be
qualified after actual execution. The broader six-format, allocation-failure,
cancellation, marker, alias and lease tests are host fixtures; the native fixture
does not substitute for that whole matrix or for interactive editor testing.

## Original three-feature host and build results (historical c61bd55)

All 22 scoped regression groups passed on clean HEAD plus owned feature changes.
After the final manager sort optimization, all six affected editor/guard/capture/
view/performance groups passed separately on both clean and preserved-overlay
source variants, with their source hashes unchanged during each run. Unchanged
core/range/usage/sampler/resolver code retains the broader regression evidence;
the final convenience runner includes the separately qualified performance group
(23 groups total). Earlier compiler/fixture refusals and corrected sources remain
preserved as historical evidence, not current passes.

Apple Clang 21.0.0 on arm64 macOS, C99 `-O1 -g -Wall -Wextra -Werror` with
AddressSanitizer and UndefinedBehaviorSanitizer, and the pinned Homebrew Python
3.14 executable were used. Four 640x512 planar secondary pages were rendered and
inspected on the host. Both the original clean planar golden and the preserved
working-layout golden pass. One final working guard invocation took 326.245s
externally while unittest reported 6.727s; the full outer delay is retained in
its log/JSON and does not establish a native CPU or response-time result.

| Historical c61bd55 candidate | PT24GEdit bytes / SHA256 | PTWorkflowTest bytes / SHA256 |
|---|---|---|
| Clean committed source | 306276 / `24f84a4092b3224a3607ea228e5e5eb2d5496a44e8b70afc2e60b9ceb585496c` | 204808 / `0e0c658aa1f8375ffe58fd079d81971ea57839b4979d00b3c0c698ff9ed1966d` |
| Preserved display overlay | 310608 / `a2c286fa47512e4276f8828a56445a7aa7db6dc7449c6d398b50ac12c36292b5` | 205320 / `5c562c6e69d3c0aff6b8afa669561fac873ad01c72a29d8560bdca15f315e5b4` |

These are successful crossbuilds only. The exact compiler, runtime inputs,
generated-source hashes, source closure, flags and binaries are archived in the
matching `core-build.json`. Compiler flags use `-m68000 -msoft-float -mcrt=nix20`
and the assembler uses `-m68000 -no-fpu`; no mandatory FPU/060 dependency is added.
The selected working candidate includes the pre-existing uncommitted display
layer; it must not be confused with the clean commit or its binary.

Exact commands are recorded in [validation.json](validation.json). The final
host runner supports `--group` for affected checks; omitting it now runs all24 groups, including incremental project validation:

```sh
/opt/homebrew/opt/python@3.14/bin/python3.14 -B tools/test_workflow_host.py --evidence-dir /private/tmp/pt-workflow-new-host-evidence
/opt/homebrew/opt/python@3.14/bin/python3.14 -B tools/build_core_tests.py --cc /Users/james1/Documents/Codex/2026-08-20/work-from-the-design-spec-v1/repo/.cache/amiga/bin/m68k-amigaos-gcc --target PT24GEdit --target PTWorkflowTest
```

The first command is the full-suite reproduction command; recorded final runs
were the22-group pre-optimization suite plus affected6-group runs. The build
command was actually run independently in both source variants. Full evidence
is in the development workspace `outputs/workflow-addendum-7ya6a7zc` alongside
saved component source/proof/review receipts and pre-edit snapshots.

### Measured host observations

The final working-overlay observation used a 1,048,576-frame 24-bit stereo
master (8 MiB active authoritative int32 storage, plus256 bytes declared padding)
and copied a 4 MiB actual current range. All values and source padding were
checked; selection/status-only idle calls reused the finished overview.

Overview: 512 steps, at most 4096 actual value reads per step; 15.085ms total host CPU and 38us maximum observed step. Copy: 1029 idle calls; 11.261ms total host CPU and 7.413ms maximum observed idle including synchronous validation/allocation. These are one-run host process CPU observations under sanitizers, not elapsed native030 latency or worst-case guarantees.

Peak tracked requested payload: 12,797,530 bytes; undo retained 12,797,530 bytes for redo. Disposal released all tracked requests (0 bytes, 0 allocations). The accounting includes fixture editor/document/master/private sampler payloads and excludes allocator headers, runtime and RSS. No native Chip/Fast peak or availability is inferred.

## Current responsiveness follow-up

The 4 October continuation adds cancellable incremental project/PCM validation
and a single reference-map pass for bulk cleanup. All 24 current scoped host
groups and six affected preserved-overlay groups pass; both exact current 68k
editor/controller candidates crossbuild. Details, measured before/after host CPU,
remaining synchronous boundaries and current hashes are in
[RESPONSIVENESS.md](RESPONSIVENESS.md) and
[responsiveness-validation.json](responsiveness-validation.json). Historical
c61bd55 results above remain preserved and are not the new candidate hashes.
Native acceptance remains NOT_RUN.

## Memory and work bounds

The editor owns two fixed overview buffers, each 1,240 signed min/max bins.
Overview steps read at most 4,096 actual PCM values; staged transaction steps
initialize/copy at most 4,096 PCM/marker bytes. Usage covers all stored events in
bounded chunks, using fixed 255-slot previews and conservative owner masks.
Manager sorting/filtering and waveform drawing never create playback caches or
upload hardware audio. Changed view summaries publish only when complete.

The current transaction and first inherited-event project/PCM validation yield
in steps of at most 4096 validation items, separately from copying or replay.
Transactions still require stopped transport, recording and affected previews.
The frontend still refuses first uncached inherited-event validation during an
active performance; same-version cached navigation uses bounded ticks. Initial
metadata/allocator callbacks and atomic commit/undo checks remain synchronous.
See [current responsiveness work](RESPONSIVENESS.md) for exact evidence and limits.
No measured 030 response-time or Chip/Fast peak is claimed by these bounds.
Cleanup can free slots while undo still owns masters: immediate released master
bytes are reported as zero. A full journal or sampler budget refuses the operation
without silently purging history or downgrading precision.

## Open inputs and target gates

The initial standalone addendum lacked its companion files. The human supplied
the complete package folder on 4 October 2026; its README_FIRST.md and
CODEX_START_HERE.txt,36-case ACCEPTANCE_TESTS.md, SOURCES.md and TEST_MATRIX.json
are now read and preserved in [package-v1](package-v1/README_FIRST.md). All six
manifest entries match; the adopted main addendum is byte-identical. This resolves
the missing-document input. The original matrix remains 36 NOT_RUN planning rows;
its applicable emulator/physical acceptance has not been executed.

The established shared harness must qualify the exact editor/controller candidate
in Amiberry first, then separately scope real A1200 workflow and playback tests.
The user has confirmed the physical Amiga/AmiGUS is available. Neither this
availability nor Safari View Only clears shared transport ownership. Fresh
ownership/recovery guards, configuration, candidate hashes, completion, exact
cleanup and explicit release remain required. No target operation, control,
reservation or recovery hold was created by this integration work.

The earlier automatic approval review rejected an AmiConnect coordination message
while destination permission was pending. That historical result remains preserved.
The user subsequently authorized coordination messages to AmiConnect, Scott and
AmiGUS, and the manual A500 session was confirmed closed. Scott's guarded release
is reported complete. None of those records grants a new ProTracker target window:
fresh candidate, peer ownership, recovery and live harness guards remain required.
Native emulator/physical acceptance remains open. Output scheduling, device RAM
capacity/order/completion and listening gates are unchanged. The addendum is not
complete until its mandatory suite and critical real 030 workflows plus playback
coexistence pass.

See [native acceptance plan](NATIVE_TEST_PLAN.md), [usage/keymap](USAGE.md), [requirement map](REQUIREMENTS.md) and
[integration contracts](INTEGRATION.md).


## Current scoped HOST refresh — 7 October 2026

At committed baseline d4966fa, 14 clean and 8 preserved-display-overlay groups
passed once with original sanitizer/assertion/output contracts. All 40 recorded
drivers returned 0 and were reaped/quiet; 80 complete raw streams and the independent
saved audit were verified. Both editor goldens and the four workflow-view image
assertions per variant remain unchanged. The 132 clean and 139 preserved-overlay
providers retain the same protected 16 originals. This is a 22-execution scoped
refresh; it is not a new all 24-group run or native/physical acceptance.

The long stereo fixture again retained 24-bit masters and bounded work: 512 overview
steps, at most 4096 values per step, a 4,194,304-byte copy and zero final requested
bytes/allocations. Exact one-run HOST CPU/requested-payload observations and
evidence tiers are recorded in
[current refresh evidence](../evidence/workflow-current-host-d4966fa/README.md).
Native 030 responsiveness, real A1200 workflows, playback coexistence and listening
remain untested for this exact refresh. Historical measurements above retain
their original candidates and scope.
