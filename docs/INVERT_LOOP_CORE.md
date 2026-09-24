# EFx clock and cursor core — dev89

The new invert_loop core implements the pinned ProTracker2.3F EFx speed
accumulator and byte-address progression without writing project PCM. It is a
prerequisite for renderer integration; EFx remains refused by the renderer.

The source rules are vendor/pt23f/replayer/PT2.3F_replay_cia.s mt_FunkIt,
mt_UpdateFunk and mt_FunkTable, also present in PT2.3F.s InvertLoop. A nonzero
speed adds its table value to an eight-bit accumulator. Bit7 causes a reset
to zero (the remainder is discarded), then advances the cursor before the byte
is inverted. End wraps to loop start. With no sample bound, a threshold still
resets the accumulator but cannot emit a mutation. EF0 disables updates without
clearing the residual accumulator. Sample binding resets the cursor but retains
speed and accumulator; offsets use a separate bound flag so offset0 is valid.

The API emits only a bounded frame index. Integration must invert the private
mono8 sample value as -1-value, preserving shared mutations between channels
using that sample. A private workspace must survive sample reloads but be
fresh for each render/stem/bounce, and must never alias project PCM. The caller
must follow the replay's exact fresh-row/effect-pass ordering; this module does
not schedule those calls. Rendering support will require actual pinned replay
byte-mutation captures, shared-sample ordering, cancellation and repeatability
checks before lifting the refusal.

Host regression derives intervals independently from the pinned assembly table
and compares 512 updates at all sixteen speeds. It also checks silent/unbound
updates, EF0/resume, range rebinding, invalid input preservation and unchanged
output index when no event occurs. These are source-derived algorithm checks,
not emulator traces, analogue audio or physical acceptance.

Validation: all 95 host tests passed in 144.769 seconds; targeted sanitized
clock test passed in 0.119 seconds. PTInvertTest cross-build passed (16,500
bytes); current build input hashes verified. No emulator window or physical
Amiga was accessed for this milestone. Evidence: evidence/enhanced-editor/dev89.

## Private PCM workspace (dev90)

invert_pcm now prepares a caller-owned, bounded mono8 sample copy and applies
clock events to that private copy. Every channel referencing one instrument
must share its workspace, so two inversions of the same byte cancel and later
notes retain mutations. Reset restores the original PCM for a fresh render;
ordinary sample reloads must not reset it. The source is never written.

Setup validates the source, classic even length, capacity and storage aliases
before changing the destination or workspace. Mutation rejects invalid cursor
ranges before changing clock or sample state. There is no allocation or global
state. Callers must retain source/storage lifetimes and keep initialized
workspace descriptors and PCM exclusive to this API during use.

Tests cover shared-channel cancellation, reset, all 256 signed8 values inverted
twice, immutable source preservation, alias/capacity refusal and invalid state
preservation. Renderer integration, workspace-bank allocation policy and pinned
replay mutation captures remain unfinished. EFx is still refused by rendering.

Integration ordering note: fresh sample reloads enter mt_CheckMoreEffects,
where EFx sets the speed and optionally updates immediately. Ordinary effect
passes enter mt_chkefx2 and update before dispatch. Delayed pattern passes must
be traced carefully rather than treating every tick as one uniform update.

Dev90 validation: all 96 host tests passed in 143.912 seconds; targeted sanitized
workspace tests passed in 0.274 seconds. PTInvertPCMTest cross-build passed
(26,648 bytes); build source hashes verified. No emulator or physical access
was made. Evidence: evidence/enhanced-editor/dev90.

## Pinned mutation snapshots (dev91)

PTInvertTraceTest is a separate 164-byte diagnostic based on the sample trace.
It retains the original 140-byte prefix, then records channel 0 wave cursor,
shared gliss/funk byte, accumulator, two reserved bytes and the first 16 loop
bytes. The byte snapshot is zeroed unless a non-null loop has at least 16 bytes.
The pinned replay still performs the mutation; the diagnostic only observes.
Shipping replay and existing trace binaries retain their prior formats.

EFF, EF8 and EFF followed by EF0 fixtures use a 16-byte loop, covering full-speed
inversion, accumulated slower updates, cursor wrap and disabling. Capture
instrument1789973006252295000 passed seven executions in 37.488 seconds. Duplicate
traces match exactly and the first 52 baseline bytes match dev38. Host assertions
check every captured cursor, speed, accumulator and mutated byte against expected
row/effect-pass sequencing. In particular, an ordinary fresh row does not add
an UpdateFunk call, while effect ticks continue the stored speed.

This establishes reference mutation evidence for these cases. Pattern delays,
multiple mutating channels and sample reloads still need capture coverage before
complete renderer acceptance. EFx remains refused in the production renderer.
The emulator window was coordinated and guarded; fresh release 06:44:33 UTC.
No physical or analogue acceptance. Evidence: evidence/enhanced-editor/dev91.

Dev91 validation: all 97 host tests passed in 144.623 seconds; targeted byte
snapshot assertions passed in 0.003 seconds. Cross-build and source manifest
checks passed; existing PTSampleTraceTest binary remains identical to dev88.

## Flow-driven mutation sequencer (dev92)

invert_sequence connects the private PCM and channel clocks to tracker flow.
On fresh rows it binds a newly selected instrument's loop without clearing the
channel's speed or accumulator. Ordinary effect passes update before command
dispatch; EFx at counter 0 sets speed and invokes the immediate update when
nonzero. Channels run in ascending tracker order and share instrument-indexed
workspace entries. Slice, unsupported loop/interpolation and missing workspace
bindings fail the staging run. Errors may follow earlier channel mutations;
callers must discard that staging run rather than publish its output.

The C test consumes every dev91 pinned byte snapshot for EFF, EF8 and EF0,
comparing cursor, accumulator, speed and all 16 mutable bytes while checking
source immutability. This validates the C sequencer, not merely a second Python
model. Production render_stream does not yet invoke it; EFx remains refused
until the workspace bank is wired through measurement and rendering. Delayed
rows, shared-channel mutation and sample reloads need extended native fixtures.

Dev92 native core validation: invertsequence1789973857232346000 passed all
three cases in 27.079 seconds. PTInvertSequenceTest is 30,724 bytes, SHA256
2459ea9cf8833720adce56c061b7fd21e7899deaaa4b2bad9e4e88fe5395bfab.
Targeted sanitized host parity passed in 0.258 seconds; cross-build passed.
The emulator window was coordinated and guarded, with fresh release 06:58:37
UTC. Evidence: evidence/enhanced-editor/dev92. No physical or analogue acceptance.

All 98 host tests passed in 154.831 seconds.

## Extended ordering reference (2026-09-24)

Shared030 capture `invert-shared-1790247850357441000` adds EE2 pattern delay,
EFf/EF8 channels sharing sample 1, and instrument-only reload. Each ran twice;
the flow-driven C sequencer matches every active channel-0 cursor, speed,
accumulator and all 16 shared loop bytes (30/18/18 checked ticks per run).
Master PCM remains byte-identical throughout. Sanitized host parity and prior
fast/slow/disable regressions pass. Evidence: `enhanced-editor/invert-ordering`.

Selective Chip allocation exposed an obsolete assumption in the diagnostic:
its pointer fields subtract the metadata address, although sample PCM now lives
in a separate allocation. The first duplicate comparison therefore failed.
That failure is retained. For these sample-1-only, no-offset fixtures, comparison
now applies one fixed relocation delta per entire capture to the 13 explicit
pointer fields. All other bytes compare exactly; cursor movement and pointer
relationships remain observable. Raw logs are retained before any comparison.
This normalization is deliberately not a general mapping for arbitrary MODs.

Completion, all four DMA-off checks and exact owned cleanup passed; the window
was explicitly released. This is emulator/reference and host-sequencer evidence.
Production EFx rendering remains refused pending private workspace integration,
allocation-failure/cancellation coverage and rendered-output validation. No
physical or analogue acceptance is claimed.

## Bounded render-owned bank

`invert_bank` prepares one instrument-indexed descriptor bank and one private
PCM allocation for the explicitly selected samples. It checks all selected
classic mono8 formats/loop bounds and total bytes before allocating, uses the
caller's allocator, and unwinds either allocation failure without publishing a
partial bank. Unselected masters, including 24-bit samples, are not copied or
converted. Reset restores private bytes from the stable masters for another
pass; release is safe twice. Masters must remain stable and outlive the bank.

The extended native-trace host test now runs through this allocator-owned bank.
Budget refusal, both allocation-failure points, selection, mutation isolation,
reset and complete release pass with address/undefined-behavior sanitizers.
The bank fixture also cross-compiles for 68030. It has not yet been run on the
Amiga; production render/Studio entry points still refuse EFx. Wiring the bank
into those entry points and testing output/cancellation remains unfinished.

## Explicit offline EFx render and verified WAV APIs

`pt_render_invert_stream` now feeds private sample descriptors to the mixer and
runs the reference-tested mutation sequencer between audio intervals. Its
explicit extra-sample budget covers staging descriptors and selected PCM; the
ordinary render workspace uses the same allocator separately. The initial
subset requires all tracks, whole mono8 forward loops with at least four loop
frames, and no interpolation or slicing. Partial tracks/stems are refused so
shared mutation dependencies cannot silently change. Existing editor/CLI and
queued Studio paths remain unchanged and continue to refuse EFx.

`pt_render_invert_file_new` uses the existing no-replace WAV publication code.
Each measurement, output and byte-verification pass starts with fresh private
copies. Exact sample/WAV checks, every allocation failure, budget refusal,
cancellation, sink failure and cleanup pass under host sanitizers. Existing
render and file regressions pass. These are explicit APIs, not yet UI wiring.

Native run `render-files-1790248715532278000` FAILED an allocation-accounting
assertion and reached a Software Failure requester (48000004). DMA is off but
Process5 remains; guest files/launcher are retained under a recovery hold, with
AmiConnect informed. No cleanup, reset, retry or physical acceptance is claimed.
The manual build omitted canonical CRT/compiler-safety flags; a corrected
build succeeds but has NOT run. That omission is a candidate explanation, not
an established cause. Source build integration now uses the canonical flags.
Evidence: `enhanced-editor/invert-render-host`.

The owner subsequently approved the shared emulator restart. The supported
lifecycle recovered bridge/guest identity; the old process was absent and DMA
off before exact retained-run cleanup. The failed run remains failed and its
snapshot is preserved. Corrected core run `render-files-1790249723400991000`
returned0: exact PCM, memory failure, budget, cancellation and sink checks pass.
That window completed, cleaned up and was explicitly released. Native diagnostic
assertions now return20 rather than creating a fatal requester. The canonical
runtime/compiler flags were restored; no claim isolates the original root cause.
Evidence: `enhanced-editor/invert-render-qualified`.
