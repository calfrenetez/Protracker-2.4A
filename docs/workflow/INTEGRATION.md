# Native 030 workflow integration

The adopted specification is [PT24G-030-WORKFLOW-v1](PT24G_030_WORKFLOW_ADDENDUM.md),
issued 3 October 2026. Its source SHA256 is
`41900a674a0024b4f83b6ff0bcf78d73d2f9153f2a4852c1ec98b3e79c4d9fb6`.
The human requested these three additions; the attachment supplies their detailed
requirements. Its deferred features are excluded. Existing master requirements,
exact scheduling decision, native 2.3F pin and accepted display work remain intact.

## Existing seams and changes

| Workflow | Reuse | Addition |
|---|---|---|
| ZT1 sample manager / cleanup | 1-based stable sample slots, all stored events, immutable sampler versions, shared chronological resource journal, existing explicit Paula audition | Conservative bounded usage scan, view-only list, version-tagged initially unselected cleanup preview, one atomic selected-only clear transaction |
| ZT2 loop / selection | Half-open frame selection, loader loop-sentinel adapter, viewport, immutable masters and journal | Validated loop navigation, staged current-range copy to truly free slot, reusable bounded waveform summary |
| ZT3 event navigation | Actual flow/pitch instrument-selection semantics, exclusive P/A/M track routing, existing sample and MIDI route pages | Structured bounded offline resolver, silent select/open commands and saved edit origin |

`pt_project` permits 255 sample slots, 256 stored patterns, 256 orders, 64 rows per
pattern, 16 tracks and 4096 slice markers per sample. There is no multisample
instrument map or MIDI Program Change table. Unknown optional extensions protect
cleanup and destination eligibility rather than being interpreted as owner-free.
Every nonzero instrument assignment counts, including instrument-only, muted,
MIDI and off-order patterns. Distinct pattern counts describe stored references,
not executed note occurrences. Slots never compact or renumber.

Masters are signed interleaved int32 elements with declared 8/16/24-bit precision,
mono/stereo and source rate. Ranges use frames, not bytes/words. Resident PCM
capacity is `capacity * sizeof(int32_t)`; undo-retained and document-shared storage
must not be presented as immediately reclaimed bytes. The existing sampler budget
and fixed-capacity journal remain authoritative. These workflows refuse pressure
rather than purging history, overwriting resources or downgrading precision.

The frontend owns task-side jobs and advances bounded steps between input polls.
No new interrupt, DMA, cache upload, MIDI send, recording hardware or output engine
is introduced. Destructive copy/cleanup requires stopped transport/preview and the
existing confirmed-release barrier. Explicit audition is unavailable during an
active performance. Read-only selection and navigation do not invoke that barrier.

## Guidance and preflight evidence

Read `docs/handover/2026-09-18-v2/README_FIRST.md`, its referenced
`CODEX_KICKOFF.txt`, repository/parent instructions, the current checkpoint,
`docs/SAMPLE_MEMORY.md` and shared harness onboarding. `CODEX_START_HERE.txt` and
the attachment's mandatory `ACCEPTANCE_TESTS.md`, `SOURCES.md` and JSON planning
companion were not found in the supplied project/iCloud locations. Their location
has been requested; no companion contents or acceptance pass is invented.

Pre-edit HEAD: `61f3b01e1bd45539bbaf85c8769cd8d5a46aca48`, branch
`feat/hardware-independent-core`. Sixteen dirty display/editor/harness paths were
saved byte-for-byte before integration. They remain protected; only owned feature
changes may enter commits. Seven existing sanitizer groups passed before editing:
sampler, slots, editor/controller plus planar image, pattern, song, slices and flow.

## Target gates

Host checks and pinned 68k crossbuilds do not qualify native interactions. The
shared 030 configuration is 2 MiB Chip / 128 MiB Fast / 68030 / no FPU / AGA, but
no new workflow candidate has run there yet. Real A1200 availability and AmiGUS
connection are human-confirmed, while fresh ownership/control/recovery/transport
clearance is still required. Safari access alone does not clear shared DevBench.
An automatic approval review rejected this task's coordination status message
because explicit cross-task messaging authorization was pending; permission has
been requested and no target operation was attempted.

Required native gates remain open: exact candidate Amiberry workflow exercises,
undo/cancel/allocation/lifecycle cases, measured long-sample latency and memory,
then separately scoped real A1200 workflow and playback-coexistence tests with
installed configuration recorded and guarded cleanup/release. Starting a binary
is not feature acceptance. Existing native output/timing failures and unfinished
AmiGUS device-capacity/order/completion and listening gates remain unchanged.
