# Sampler ownership for typed reader lineage

`sampler_paula_lineage` is a separate opt-in adapter for `scheduled_lineage`.
It prepares independently retained Paula representations for TRIGGER, CONTROL
and STOP batches. The legacy future TRIGGER, prepared-owner and scheduled-output
APIs remain unchanged. This adapter supplies software ownership and exact-target
contracts. It does not implement a native output backend, IRQ, DMA, song scheduler
or a physical timing capability.

## Preparation and exact targets

Open a dedicated pool with an explicit owner limit from one to eight, a controller
allocation budget and a separate Chip-representation byte budget. Each owner has
one to four requests with distinct stable Paula slots. Owner tokens are unique
within a pool and are never reused; exhausted tokens or budgets refuse new work.
No owner, master or cache allocation occurs through backend activation callbacks.

TRIGGER requests have no predecessor or reader key. The adapter uses existing
incremental sampler master and Paula cache jobs; one step performs one transition,
up to 4096 master-copy bytes, or up to 256 Chip-conversion bytes. Views stay hidden
until every request is ready. A trigger batch may address only the retained signed
8-bit cache's aligned word extent, never protected master or controller bytes.

CONTROL and STOP requests require a genuine positively observed ACTIVE key from
an original TRIGGER registered in this pool, with the same lineage queue, stable
slot, sample and selected source channel. The key includes queue/session,
generation, ticket, action, owner and non-reused activation serial. Enqueue success,
absence of retirement, a pending trigger or identical cache bytes cannot create
an ACTIVE key. CONTROL/STOP cannot target another CONTROL/STOP as an origin.

Preparation temporarily borrows the original owner's metadata. The original may
be positively retired during preparation, but its resources cannot be disposed
while borrowed. The command acquires its own current master pin and identical
cache slot/serial lease. It then drops that borrow and clears both raw predecessor
pointers before READY. A command can therefore retain storage after its original
holder closes, but that retention grants no permission to control a retired reader.

Every preparation step and enqueue rechecks the complete original key. The core
queue independently revalidates all targets at publication. The backend must
atomically compare the whole exact target set again in the original frame window
before any action takes effect. A task-side snapshot cannot prove that a reader
will still be active at that future instant. Current-owner callbacks check held
resources and batch geometry only; they do not recursively query the queue.

## Refusal, retirement and lifecycle

Before transfer, invalid aliases and refused batches preserve the complete owner,
input and caller output. Valid stale preparation or a job failure releases only
that owner's partial resources and preserves the readiness output on failure.
Changed routes, sampler generation, source descriptors or project headers refuse
before traversing changed tables. Requests and action arrays may differ in order:
matching uses the stable slot and captured source, with whole-batch validation.

After queue OK, the owner is LIVE. Direct cancel and close refuse until the core
positively retires that exact ticket's complete retained-reader domain. A locally
unsubmitted ticket can retire with the core's explicit local provenance. Submitted
CONTROL cancelled before activation leaves its original reader ACTIVE; STOP pending,
issued or DRAINING is not all-reader retirement. Each ticket retains its own full
master/cache references independently of the other tickets. A matching retirement
of one ticket never releases another ticket's references.

Unknown or mismatched retirement envelopes retain all references. A valid exact
all-reader retirement with invalid command/timing classification may still release
that ticket once, while the terminal callback latches pool failure and refuses
further preparation. Callback reentry similarly fails closed and defers unsafe
resource disposal. No automatic retry, force-close or inferred hardware stop is
provided. Retired or cancelled handles still occupy an owner slot until explicit
close. Unpinned caches may remain evictable within the Chip budget until eviction
or pool close; retirement is not required to free that allocation immediately.

## Borrowed storage and output protection

Sampler, project metadata, original external sample backing and the lineage queue
must satisfy their borrowed lifetime contracts through owner/pool close. Live
source/header changes make work stale. Caller contexts and opaque handles cannot
be copied or fabricated. Operations and callbacks are serialized on the task side.

The adapter checks actual full pool/owner/job/version/backing/PCM/marker/cache
capacities, sampler controls and adopted table capacity. Public output/input guards
also protect captured project arrays and extension bytes without scanning spare
sample values. Released private captures are not retained as ghost spans. The
queue owner receives the selected genuine held source and control spans; that
list is not a claim that every borrowed project table is an independently pinned
reader resource. Caller outputs and control inputs must remain disjoint from
opaque queue/backend controls and allocator/Chip callback contexts whose extents
this adapter cannot discover. Direct `reader_key` forwards the actual external
output to the core so its own queue/backend alias guards remain effective.

## Verification scope and remaining work

The shared fixture uses the actual sampler master/cache jobs and typed core queue
with ordinary host allocations and a simulated timestamp backend. It covers all
8/16/24-bit mono/stereo masters with nonzero precision bits, bounded copy/conversion,
independent command pins, reversed two-action order, exact replacement after
publication, retirement before/after READY, every pending cancellation phase,
allocation failures, aliases and complete before-images, finite pressure, stale
headers, reentry and invalid-terminal propagation. Legacy future-owner and core
lineage sanitizer regressions run separately against the same frozen export.

Those checks qualify a software contract. Host allocations do not prove Chip
memory class, Paula DMA, hardware voice stop, an interrupt deadline or listening
acceptance. A native adapter and exact output capability remain unimplemented.
The fixed eight-event full-reader domain intentionally refuses pressure. The
separate opt-in [persistent-reader binding](SAMPLER_PAULA_READERS.md) now keeps genuine
sampler pins independently of recyclable command holders and passes host checks
plus a pinned portability build; it remains native NOT RUN. Production song and
actual backend integration remain unfinished.

## Saved host and compiler qualification

Three sanitizer groups passed from the complete frozen author export, with
47 full-M dependency checks,56 canonical inputs and105 system fingerprints. A clean
current committed export plus the exact five adapter files and standalone builder
passed one 68k compile/link:33 RC0 commands,22 ordered translation units,51 canonical
inputs,32 SDK inputs and seven runtimes, with active SDK assertions and the full
completion marker once. The resulting 151,592-byte HUNK has NOT RUN. A 2 MiB test
stack is provisional; neither adequate stack nor native/Exec memory placement,
DMA, IRQ, hardware timing or physical/audio/listening acceptance follows.
The original frozen host documentation and all prior development failures remain
archived separately. See [saved host/compiler records](../evidence/enhanced-editor/sampler-paula-lineage/README.md).

## Exact cache source binding — 16 October 2026

The typed lineage owner now checks the actual cache representation key against
its held sample and explicit stereo side, including signed8 format and padding,
before publishing a view or enqueueing a command. A live lease with matching
version, address and byte count is insufficient. This uses the existing internal
source predicate after current bridge validation; it adds no ownership grant or
PCM scan and never treats cache refusal as confirmation that a reader stopped.

A separate genuine fixture covers 198 cases: 36 valid controls and 162 wrong
sample, side, bit-depth, byte-order or padding refusals across 8/16/24-bit mono
and stereo masters, with trigger/control/stop owners both ready and active in
the software backend. Keys come from the production packer; original pins and
leases remain genuine through independent per-ticket retirement. Refused outputs,
cache and master bytes and backend observations stay unchanged. Full close leaves
zero explicit owned allocations. The unchanged baseline reproduces the missing
refusal; eight host tests in seven groups pass with ASan/UBSan and assertions.

The original full lineage fixture also passes a fresh pinned Amiga compile/link
check. The new 198-case fixture is not included in that HUNK. Neither fixture has
run in Amiberry or on the A1200 in this qualification; Exec memory placement,
device/IRQ/DMA, actual voice stop, output timing and listening remain separate.
See [the scoped host/build result](../evidence/enhanced-editor/lineage-source-binding/host-build.json).

## Separate three-source preparation owner — 16 October 2026

`prepared_paula_sources.c` now provides a native-eligible, normal-task owner for exactly three distinct, already-established mono masters. It acquires genuine persistent master references through the existing pin-job begin and completion steps, then acquires selective signed8 Paula caches through the existing Chip jobs. A HIT or final LOAD transfers a lease; PENDING transfers none and retains the job's separate temporary master reference. Each call performs at most one child transition or one existing 256-byte upload chunk. Authoritative 8/16/24-bit masters and their precision remain unchanged. Initial document promotion, a new allocator binding and general-song preparation are outside this API.

Fresh generation, bridge, route, project-table, descriptor and cache/source checks gate preparation and each READY return. Numeric status exposes no source, PCM or cache pointer. Full owner-capacity output alias checks precede every getter write, including stale-state and reentry fault latches. Cancellation permanently ends preparation and disposes only never-exposed resources, one original job cancellation or owned pin/lease release per call. It uses captured owners rather than former stale sample tables. Successful cleanup retains the original failure; refused or uncertain release/reentry retains the surviving owners in HELD. No reset, forced close or reference-count rollback is supplied.

The first HOST integration group passes strict warnings, assertions, ASan and UBSan across 22 actual translation units and 53 copied project dependency bodies. Real sampler and cache jobs cover 24 load transitions, nine HIT transitions, cancellation at all 25 cuts from initialization through READY, and 36 stale generation/route/descriptor/table cleanup cases. Packed bytes, 8/16/24 master values and enhanced-project save bytes match exact oracles. Constructor/output aliases, full-capacity tails, a protected page, read-only current/stale owners, allocation limits and cache-close refusal are covered. Clean fixtures end all allocations; two deliberate forbidden-callback fixtures retain genuine remaining resources and complete allocator contexts until process exit. They are not reclassified as successful cleanup or repaired by restoring ownership snapshots.

This owner is not wired to the inhibited provider or native PLAY. It exports no backend arm/apply, external borrow, command/reader retirement, quiet or source-release certificate. Later output integration must retain all three dependencies until actual reader retirement and zero command references independently permit release; this preparation API proves none of that. Native compile/run, actual Fast/Chip placement, IRQ, DMA/device quiet, exact output timing and listening remain unqualified. Allocation, eviction and metadata work can remain synchronous; a byte bound is not a latency or WCET guarantee. See [HOST evidence](../evidence/enhanced-editor/prepared-paula-sources/host.json).


## Three-source native RAM probe build —16 October2026

A separate18-unit probe now passes the pinned Amiga compiler/linker check from
a54-file private export, with51 actual project dependency bodies,42 selected
SDK bodies and seven runtimes. Assertions stay enabled. Its131,096-byte HUNK
(SHA256 `ef6259a77f4da61ab0c69ad9811dc6e9367423d8e467d09097b9a932f1c1049e`,
CRC `856A177A`) has not yet run in Amiberry or on the A1200. The first build's
missing-SDK-pin refusal is preserved; the successful separate build first
established the complete actual18-unit dependency set.

The probe promotes genuine distinct mono8/16/24-bit masters, each513 frames,
through the strict checked Fast allocator. It prepares three selective514-byte
Chip caches using the unchanged512KiB reserve policy, then checks cold loading,
a separate-owner cache HIT and cancellation after0/1/2/3/4 transitions. Seven
distinct Fast owner arenas stay allocated until final cleanup; none is reset or
recycled. Allocation flags plus base/final-byte TypeOfMem observations check
placement. Exact master/marker and in-memory project-encoding comparisons and
zero explicit Fast/Chip ledgers gate the sole PASS marker. Any uncertain
cancellation or refused restoration retains the entire process immediately.

These are source assertions and build checks until exact execution. This native
subset does not replace the HOST25-cut/36-stale/fault matrix. No audio, output
activation, IRQ, DMA, device stop, card RAM, timer or filesystem save is exercised.
Per-function stack records do not establish aggregate stack or execution-time
bounds. See [pinned build evidence](../evidence/enhanced-editor/prepared-paula-sources/native-build.json).


## Three-source RAM probe Amiberry pass —16 October2026

The exact131,096-byte candidate above now passes once in the shared030 Amiberry
profile: RC0, sole165-byte completion line and all eight execution-custody files.
Its genuine8/16/24-bit master/owner Fast assertions, three selective Chip caches,
cold/HIT acquisition, five cancellation cuts, master/marker/in-memory-save
preservation and final zero explicit accounting all complete. Seven distinct
owner arenas are retained until cleanup; no reset or address recycling is used.

The packet passes24 runner and27 service HOST tests, plus15 normal-entry checks
and separate validation. Exact build/source closure and fresh shared ownership
checks precede the run. Task and both RAM namespaces are independently absent,
saved launcher files are restored, public release and only-owned-service exit
pass, and a separate fresh idle check confirms closure. Original90/140/250-second
bounds and observed1MiB task Stack remain. The104.52-second runner observation
includes transport and cleanup; it is not native cost or a performance bound.

This qualifies the selected pre-exposure RAM assertions on the emulated CPU and
Exec allocator. A1200 execution, reader/command transfer, native output, physical
DMA/device quiet, musical timing and listening remain unqualified. No playback
provider inhibition was removed. See [exact emulator evidence](../evidence/enhanced-editor/prepared-paula-sources/emulator.json).
