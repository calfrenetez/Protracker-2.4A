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

## Final host and build results

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

| Exact final candidate | PT24GEdit bytes / SHA256 | PTWorkflowTest bytes / SHA256 |
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
host runner supports `--group` for affected checks; omitting it runs all23 groups:

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

## Memory and work bounds

The editor owns two fixed overview buffers, each 1,240 signed min/max bins.
Overview steps read at most 4,096 actual PCM values; staged transaction steps
initialize/copy at most 4,096 PCM/marker bytes. Usage covers all stored events in
bounded chunks, using fixed 255-slot previews and conservative owner masks.
Manager sorting/filtering and waveform drawing never create playback caches or
upload hardware audio. Changed view summaries publish only when complete.

Initial transaction project/PCM validation is synchronous and requires stopped
transport, recording and affected previews. Initial inherited-event flow
validation may scan PCM; the frontend refuses that first validation during an
active performance, while same-version cached navigation uses bounded ticks.
No measured 030 response-time or Chip/Fast peak is claimed by these bounds.
Cleanup can free slots while undo still owns masters: immediate released master
bytes are reported as zero. A full journal or sampler budget refuses the operation
without silently purging history or downgrading precision.

## Open inputs and target gates

`README_FIRST.md` and its referenced `CODEX_KICKOFF.txt` were read.
`CODEX_START_HERE.txt`, mandatory `ACCEPTANCE_TESTS.md`, `SOURCES.md` and the JSON
planning companion were absent from the supplied/scoped project and iCloud paths;
their location has been requested. The tests in this change are derived from the
supplied detailed addendum, and no missing companion suite is represented as passed.

The established shared harness must qualify the exact editor/controller candidate
in Amiberry first, then separately scope real A1200 workflow and playback tests.
The user has confirmed the physical Amiga/AmiGUS is available. Neither this
availability nor Safari View Only clears shared transport ownership. Fresh
ownership/recovery guards, configuration, candidate hashes, completion, exact
cleanup and explicit release remain required. No target operation, control,
reservation or recovery hold was created by this integration work.

Automatic approval review rejected the attempted AmiConnect coordination status
message because explicit authorization to message that separate task was pending.
The requested messaging permission remains unanswered; it has not been bypassed.
Consequently native emulator/physical acceptance remains open. Existing output
scheduling, device RAM capacity/order/completion and listening gates are unchanged.
The addendum is not complete until the mandatory suite and critical real 030
workflows plus playback coexistence pass.

See [native acceptance plan](NATIVE_TEST_PLAN.md), [usage/keymap](USAGE.md), [requirement map](REQUIREMENTS.md) and
[integration contracts](INTEGRATION.md).
