# Responsive workflow validation follow-up

The human approved this continuation of the three existing native workflows.
Authoritative 8/16/24-bit mono/stereo masters, allocation budgets, chronological
undo, exact output scheduling, the pinned classic baseline and accepted display
layout are retained. No deferred ZooperTracker feature or new output engine is
included. Current emulator and real A1200 acceptance has not run.

## What changed

The existing synchronous project validator is unchanged. Its new caller-owned
incremental counterpart performs metadata-only begin, then at most 4096 semantic
items per step. Each item is a bounded endpoint/order/sample descriptor/active
PCM value/slice/event/extension check. Fixed header/current-sample checks are
additional; 4096 items does not mean 4096 total bytes or a wall-clock deadline.
Padding remains protected against output aliases without being read as PCM.
Results publish only after the complete current semantic pass; cancellation
never dereferences former project arrays. Borrowed storage must stay alive and
immutable between calls and all edits must change revision or generation.

Cleanup/copy validates before staged promotion or selected-range copying. It
returns to input polling after each validation step, including the completion
step; copied PCM/marker blocks retain the existing 4096-byte bound. Commit checks
complete/current state plus ownership/history guards without a new full PCM scan.
The first inherited-event request similarly prepares flow in validation steps,
then starts bounded replay on a later poll. Same-version reset reuse and exact
flow/pitch semantics remain unchanged. No generic trusted validation flag or
public unchecked flow initializer is introduced.

The editor advances one usage, transaction, resolver or overview PCM phase per
idle call. Busy ownership cancellation precedes those phases even with a pending
usage scan. The first uncached inherited-event pass remains refused during an
active performance; this work does not grant safe live-reader ownership.

Cleanup now constructs one 255-slot reference map over all stored events per
atomic apply instead of repeating the scan for every selected slot. The maximum
geometry fixture clears 254 selected slots with one 262144-event pass, then proves
late-reference and redo-reference refusal preserves slots, masters and history.
The pass includes off-order, muted, instrument-only and last-track references.

## Host evidence

The clean combined candidate passed all 24 scoped sanitizer/controller/format
groups with its code hashes unchanged during execution. Component tests cover
six precision/channel formats, budgets 1/7/4096, late invalid PCM/slices/events/
extensions, private partial capabilities, wrapped/capacity aliases, tag/header/
current-sample changes, genuinely released former arrays and safe cancellation.
The maximum metadata case uses 255 slots, 4090 extensions, 256 patterns and 16 tracks.
Sixteen previously captured native flow traces were replayed against this code
on the host; that is semantic regression evidence, not a new emulator run.

Actual evidence is retained under:
`/Users/james1/Documents/Codex/2026-09-18/rev/outputs/workflow-responsive-hxlqain9`.
Exact source closures, commands, compiler, products and source-before/after hashes
are recorded in `host-clean/report.json`, component receipts and benchmark
`record.json` files. The original c61bd55 benchmark is preserved separately.

### Same-workload host observation

Both runs use Apple Clang 21 C99 `-O1` with ASan/UBSan, a 1048576-frame 24-bit stereo
master (8388608 active bytes plus 256 padding bytes), a 4194304-byte current-range
copy and the same allocator budget. These are single-run Mac process CPU and
requested-payload observations; no native latency, RSS or Fast/Chip measurement
is inferred. The copy fixture's idle-call guard was adjusted for additional
validation calls; its workload, value/range/padding checks remain the same.

| Observation | Original c61bd55 | Current follow-up |
|---|---:|---:|
| Copy idle calls |1029|1543|
| Maximum observed copy idle CPU time |7.201 ms|0.180 ms|
| Total observed copy idle CPU time |10.277 ms|19.508 ms|
| Overview steps / maximum values |512 /4096|512 /4096|
| Peak tracked requested bytes |12797530|12802114|
| Sampler bytes retained for undo/redo |4197728|4199944|
| Final tracked allocations / bytes |0 /0|0 /0|

The new idle maximum was smaller in this observation and total CPU was higher.
No worst-case or real 030 performance guarantee follows. The validator workspace
is 2216 bytes on this host; the editor/transaction combination added 4584 requested
bytes to this workload's peak. Both runs preserve exact source/copy values and
release all owned requests on disposal.

## Remaining synchronous work and acceptance

Begin still checks at most 255 sample and 4090 extension descriptors and calls the
existing allocator synchronously. Commit/undo retains a synchronous atomic event
reference pass and fixed per-slot/journal checks. Existing unrelated save/undo/
legacy mutation APIs remain synchronous. Cancellation happens between polls, not
inside an allocator, copy primitive or atomic publication boundary.

Both pinned 68k editor/controller candidates crossbuilt successfully with
`-m68000 -msoft-float -mcrt=nix20` and the pinned no-FPU assembler. The six
affected editor/guard/capture/view/performance groups also passed against the
original preserved display overlay; all 16 protected paths remained byte-exact.
Compilation is not native execution. All mandatory native keyboard/mouse, long-sample
response/memory, format/route and playback-coexistence gates remain unqualified.
No target operation/control/lock/reservation or recovery hold was created here.
The current peer-reported failed recovery hold has not been independently cleared.
Automatic approval review rejected a coordination status message because explicit
permission to message that separate AmiConnect task was pending. Neither broad
testing authority nor Safari viewing availability overrides that decision.

The complete supplied package is preserved byte-for-byte in `package-v1`; all
six manifest hashes match and its main addendum matches the previously adopted
specification. `acceptance-status.json` is the separate current coverage/status
record; the original 36-row planning matrix remains unchanged.

## Current exact crossbuild candidates

| Source variant | PT24GEdit bytes / SHA256 | PTWorkflowTest bytes / SHA256 |
|---|---|---|
| Clean owned candidate | 310288 / `a34b60666e1d103edf3e9a0afc556849e111c4749a842e7594fa05e830157a07` | 210032 / `8d719d838e523c193e68c0d44760095c017296b9879be99241a0a3150acd0aa2` |
| Preserved original display overlay | 314620 / `2d65bb41890090e6b1c6dd072b8c0f164e59b23b316bc1c8702b8b2938f43636` | 210544 / `ddc33ab9299e071b4a291b01fd3717cd21ccd92848ac7754d7b5b4db5ef29722` |

These replace the historical c61bd55 binaries as candidates for the next
coordinated test window. Source/build/runtime/generated-input hashes and exact
commands remain in each archived `core-build.json`; they are separate from host,
native interaction, physical device and listening acceptance.

See [current machine-readable evidence](responsiveness-validation.json),
[separate acceptance status](acceptance-status.json) and
[native acceptance plan](NATIVE_TEST_PLAN.md).
