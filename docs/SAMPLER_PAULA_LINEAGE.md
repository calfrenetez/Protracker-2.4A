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
