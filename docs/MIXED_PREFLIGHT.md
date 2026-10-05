# Incremental mixed-backend capability preparation

`mixed_preflight` checks one shared Paula/AmiGUS sequence before promoting any
master or allocating/uploading a playback copy. The supported geometry, global
tempo/effect flow, backend rules and strict playback deadline policy are unchanged.

The serialized owner uses `pt_mixed_preflight_begin`, `step`, `take` and `close`.
Begin validates the project and static metadata, copies options, capabilities and
format, and owns two bounded allocations: analysis workspace and sequence. These
initial validation/scans remain synchronous and outside playback deadlines.

Each later step does exactly one analysis phase:

- At most 256 timeline measurement ticks.
- One interval `next` operation.
- At most 256 consumed frames.
- One completed command plan and its Paula/AmiGUS capability checks.

The analysis makes no master pins, derived copies, uploads, voice callbacks or
PCM output. It accumulates source masks privately; pending/failure reports expose
zero masks. A successful complete traversal publishes both backend masks and
preserves global diagnostic action/channel indices. A late refusal publishes no
partial success and cannot transfer its sequence.

Take rewinds and transfers the same audited sequence once, without allocation or
remeasurement. The caller owns that sequence afterwards. Close releases untaken
sequence/workspace storage at any phase and is idempotent. Project arrays and PCM
remain borrowed and immutable throughout analysis and transferred playback;
callbacks must not edit or reenter. The synchronous compatibility wrapper drives
these same bounded steps until completion, so it remains synchronous as a whole.

`pt_mixed_owner_prepare` now advances one analysis phase per call before any
source promotion. Only complete successful analysis allows the existing bounded
master-promotion phase. Stale-source failure and owner/editor cancellation close
the analysis before the existing reader/quiescence barriers; uncertain barriers
continue retaining the enclosing owner and engine tokens. The native editor
advancement seam can interleave calls while preparation remains unfinished.

Host sanitizer fixtures cover synchronous/incremental report parity, progress
bounds, hidden partial masks, late backend refusal, one-time sequence transfer,
allocation failures and cancellation during measurement, traversal and completed
untaken analysis. Mixed owner/editor tests cover cancellation and stale-source
refusal before promotion, pending closure barriers and existing transport behavior.
Progress bounds are not a wall-clock guarantee. Native timing, physical capacity,
transfer completion, voice-stop and output/listening acceptance remain separate.

## Optional bounded INITIAL setup

`pt_mixed_preflight_setup_begin`, `step`, `get`, `transfer` and `cancel` add an
independent initial-preparation path. Begin scans metadata/full declared extents
and allocates two fixed ordinary controls: the genuine opaque renderer startup
and the future mixed analysis owner. It reads no PCM, order, event or slice
values and acquires no master pins, cache, upload or backend references.

Each setup step advances one genuine renderer phase with work 1..4096. Fixed
header/currentness checks and finite metadata/alias guards are outside that item
budget. Initial READY means complete original-project validation and renderer
static readiness; it is neither `PT_MIXED_OK` nor a published backend source mask.
The supported mixed rates remain 44100/48000; row-range restore and unsupported
selected routes still refuse.

Transfer consumes the actual completed startup, makes one fixed sequence
allocation and publishes the actual mixed audit still `PT_MIXED_PENDING`. There
are three allocation calls and briefly three live controls in total; successful
transfer leaves analysis plus sequence. Existing mixed steps then perform the
complete two-route timeline audit. A successful take transfers the SAME checked
sequence, rewound once. Private renderer reset templates avoid repeated semantic
validation during transfer, measurement restart and rewind.

CAPACITY retains READY for explicit retry. Normal pending/alias/capacity refusals
preserve caller outputs. Cancel is idempotent and can consume pending, completed
or failed initial ownership; a newly observed release-callback fault can consume
the handle to NULL while reporting FAILED. Checked actual-audit entrypoints
reject closing reentry. Closing the empty analysis after a successful take still
releases its control. Cancellation/close use fixed captured controls and table
extents rather than walking former source descriptors; caller handle slots must
be genuine, alive and disjoint.

Original options, allocator, caps, format and previous-map argument storage stays
alive and immutable through initial transfer/cancel. After transfer the actual
audit owns copies, so those argument structs may be reused; copied callback
functions/context remain callable until every resulting owner closes. Project,
tables, full master capacities, markers and extension storage stay borrowed and
immutable through transferred sequence close, including callbacks. INITIAL uses
caller revision/generation tags; valid selection-cursor movement is permitted.
Source edits during calls are forbidden. Allocators return fresh aligned disjoint
storage: aliases to finite recognized guarded extents are refused without being
released as fresh ownership. Unenumerated opaque-context extents remain caller
responsibility. Begin callback reentry is caller-serialized before an owner
exists; later observed reentry latches failure. Existing lookahead lifetime rules
still apply.

Legacy mixed begin/preflight retain their synchronous initial scans. This optional
setup is not wired into `mixed_owner` or editor PLAY: its initial sync scans and
post-audit pin promotion require a separate immutable-source borrowing solution.
Exact scheduling is unchanged. Item/allocation bounds are not wall-clock, IRQ,
placement, backend activation or hardware acceptance.

Validation record: five ASan/UBSan groups passed once against an isolated committed baseline with the four sealed source/fixture additions. The new fixture covers 18 master-depth/cache-format/mode parity combinations, budget bounds, phase cancellation, stale inputs, allocation/reentry/alias refusal and the same checked sequence transfer; the existing mixed preflight/owner and Paula/renderer startup fixtures also passed. This is host software evidence only.
Native compiler validation: one pinned m68000/soft-float compile/link passed (37 translation units, 46 commands, 117 dependency files). The 226,688-byte `PTMixedSetupTest` HUNK has SHA256 `171f3b7b42b659093bc24b410d6affb96668527dbeb624efb1a2251b18b97530`. No HUNK execution or aggregate native stack qualification occurred.
Native/emulator/physical execution and timing/output acceptance remain NOT RUN
for this optional setup.

Saved source bindings, raw first outcomes and separate reviews are under
`evidence/enhanced-editor/mixed-initial-setup/`. The earlier unsupported-rate
fixture oracle was corrected before execution; its first sealed version remains
NOT_RUN. Existing shared recovery holds and physical cleanup obligations remain
unresolved; no target or shared service was accessed.
