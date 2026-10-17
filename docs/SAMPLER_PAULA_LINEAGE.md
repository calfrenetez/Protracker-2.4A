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


## Unpublished three-source lifetime bridge —16 October2026

The explicit `PT_PREPARED_PAULA_ENABLE_HOST_LIFETIME=1` HOST option adds a
separate lifetime bridge to the existing three-source preparation owner. It opens
an empty genuine readers queue before claiming sources. Fresh admission then
reserves the original owner and transfers one command reference plus one reader
domain that retains all three genuine master/cache dependencies. The actual lower
enqueue result establishes custody; callers cannot supply a transfer or quiet
boolean. Public source cancellation returns BUSY while that custody remains.

This first slice never publishes the command. An actual core local stop consumes
command then reader references; those callbacks record consumption without freeing
sample resources. Only after the outer stop returns does a serialized task drain
release at most one captured original child per call. Six children and a final
completion take seven calls. The queue pointer is consumed before its allocator
release callback, so free-then-reentry cannot resurrect or release it twice.
Failures retain surviving ownership and complete callback contexts; there is no
forced cleanup, count repair, retry of a refused release or source reset.

Callers supply complete accessible callback-context extents and retain original
objects until close or retained fault. These lengths are a caller contract;
addresses alone cannot establish them. Full source/lifetime capacities, backend
controls and declared contexts are protected from output/allocation aliases.
Stale-source cleanup uses the captured original ledger and does not restore an
old generation or sample table to make release succeed.

The new 25-unit integration and separate original22-unit default-OFF regression
pass assertions, strict warnings, ASan/UBSan and exact output checks. The genuine
mono8/16/24 fixture checks master, marker and saved bytes through transfer/local
stop/drain, stale cleanup, constructor cancellation, reentry and twelve alias
refusals. Positive close is followed by a protected inaccessible lifetime arena
before source get/cancel. Six full fixtures clean normally. Two deliberate fault
fixtures remain reachable: one has surviving source obligations; the other keeps
its context after a consumed raw queue and finished source ledger. Neither is
reported as successful full cleanup. All backend callbacks, including the clock,
remain unused. Selected tool/SDK operands are checked; complete system-header
and dynamic-library provenance are outside this qualification.

The option is OFF by default, with the original owner token stream preserved;
actual default preprocessing excludes lifetime fields, references and header.
The new bridge explicitly refuses native compilation. Existing native provider
inhibitions remain. Published backend retirement, R-first callback order,
quiet/device stop, output activation, native/physical integration, exact musical
timing and listening are still unfinished. This component is not active-voice
hardware acceptance. Earlier native RAM results remain their own pre-exposure
scope; current-source default native rebuild and physical promotion are separate.
See [scoped HOST evidence](../evidence/enhanced-editor/prepared-paula-lifetime/host.json).


After the HOST-only bridge was added, the default18-unit native RAM probe also
passes a fresh pinned build (51 actual project dependencies,42 SDK bodies and
seven runtimes). Every native dependency/preprocessed body excludes the bridge.
Its131,096-byte executable is byte-for-byte identical to the previously
Amiberry-qualified `ef6259a7` / `856A177A` candidate; that exact-byte evidence is
reused without another native run. This does not qualify the new HOST bridge on
Amiga. Physical testing remains unrun. See [default build and byte identity](../evidence/enhanced-editor/prepared-paula-lifetime/native-default-build.json).


## Published three-source cancellation bridge —16 October2026

The same explicit HOST-only option now supports one actual core publication,
one stop that closes admission, and one cancellation attempt per original command
and reader domain. Publication rechecks the original source ledger while holding
it busy through authentic current callbacks. The original frame100 and half-open
window `[2184,2205)` remain unchanged; no fallback or rescheduling is supplied.
The previous unpublished path and its oracle remain intact.

The backend model borrows the real submitted event and reader identities. Before
reader retirement it removes all actual data, span, domain and future-activation
references. Reader-first receipt collection still leaves the genuine core's
reader callback deferred until command references reach zero. Later task drain
requires closed admission, completed outer calls, both actual release callbacks,
zero original core held counts and no structural uncertainty. Callbacks do not
free sample resources; seven later one-child drain calls dispose captured owners.

An authenticated terminal receipt with a bad semantic classification records a
sticky failure while its actual release still consumes that domain. Both genuine
domains can therefore finish and permit captured cleanup without hiding the
failure. Pending, wrong envelopes and reentry retain unconsumed or uncertain
ownership. No failed cancellation is retried, source generation restored, private
owner flag edited or forced release supplied.

The new25-unit sanitizer fixture passes17 cases:11 complete and six deliberately
retain their complete contexts. Both orders cover normal cancellation, stale
sources and bad classification in either domain. Pending, wrong-envelope and
reentry cases retain in both orders. Three entry guards cover fresh staleness
before publication, submit-zero without retained backend references, and a clock
exactly at the first boundary. Genuine mono8/16/24 masters, markers and save bytes
remain exact through retention and deferred cleanup. The included donor supplies
acquisition helpers; its renamed main and old cases never execute or count here.
Original unpublished25-unit and default-OFF22-unit sanitizer groups also pass,
with unchanged oracles. The canonical new Python wrapper passes separately.

Default mode stays OFF, native compilation of the lifetime option remains
inhibited, and the default18-unit native RAM probe rebuilds to the complete same
131,096 bytes previously qualified in Amiberry. That byte identity reuses only
the original pre-exposure RAM evidence; it is not a new runtime test of this HOST
bridge. ACTIVE/adopted playback, IRQ delivery, hardware quiet, output timing,
A1200 execution and listening remain unqualified. See [HOST cancellation evidence](../evidence/enhanced-editor/prepared-paula-lifetime-published/host.json)
and [default native identity](../evidence/enhanced-editor/prepared-paula-lifetime-published/native-default-build.json).


## HOST adopted-reader cancellation —16 October2026

A separate eight-case HOST fixture now exercises the same unchanged API after
genuine model adoption. A serialized model dispatch validates the original event
and geometry at synthetic tick2184, inside the unchanged `[2184,2205)` window.
It captures persistent reader geometry independently of command storage. In
command-first cases it actually reads the first1026-byte Paula cache after the
core has detached and cleared the command. One reader retains all three genuine
mono8/16/24 master/cache dependencies; this is not three active voices.

Both retirement orders and stale-after-adoption cleanup pass without restoring
the generation. Reader-first receipts may first expose ADOPTED+RETIRED to the
core; no ACTIVE/key permission observation is claimed in that order. Actual
reader callbacks still wait for zero command references. All geometry and source
borrowing end before the backend returns retirement, and seven later bounded
drain calls release captured resources with exact master/save preservation.

The strict25-unit ASan/UBSan fixture passes five completing and three deliberately
retained cases. A bad classification in an authentic retired-reader envelope
still permits real domain consumption and later cleanup while preserving
BACKEND failure. Pending and uncertain replies retain the actual active reader.
A wrong envelope ends backend borrowing but cannot end the core obligation.
Those three complete contexts stay reachable without retry, repair or forced
cleanup. The canonical Python wrapper passes separately. Production code and all
previous fixtures/oracles remain unchanged.

This is ordinary HOST model ownership acceptance. Synthetic dispatch, ADOPTED
receipts and ordinary RAM reads establish no native backend, IRQ, real musical
timing, audio output, hardware quiet, physical or listening acceptance. Existing
default-OFF and native compilation/provider inhibitors remain. See [the scoped
HOST evidence](../evidence/enhanced-editor/prepared-paula-lifetime-active/host.json).


## Native TASK unpublished RAM partition —16 October2026

A separate explicit `PT_PREPARED_PAULA_NATIVE_TASK_PROBE_ONLY=1` partition now
compiles the genuine three-source lifetime adapter for one ordinary serialized
TASK RAM probe. It is mutually exclusive with the HOST option, requires the
real m68k/AmigaOS compiler target and checked ABI widths, and compiles only the
shared unpublished path. The original HOST-on-native rejection and default-OFF
owner path remain. Native publication/cancel wrapper and friend APIs are absent;
this supplies no native output provider or IRQ delivery.

Four original HOST fixtures retain their exact oracles and pass strict sanitizer
regression. A separate actual cross-compiler matrix passes24 cases (eight
positive,16 meaningful refusals) and three object boundaries, including absent
publication/collection declarations and call edges from the adapter objects.
These are static eligibility checks, not native ownership or quiet proofs.

The new21-unit probe compiles and links with58 actual project dependencies,
42 selected SDK inputs and seven runtime inputs. Its planned runtime sequence
retains three genuine8/16/24 master/Chip-cache dependencies through one original
unpublished1C/1R transfer, actual local C-then-R stop with zero backend calls,
and seven later bounded drain calls before queue close. The native assertions
have NOT_RUN: neither Amiberry nor physical execution is claimed for the new
152,856-byte candidate. A fresh default18 build independently reproduces the
entire old RAM executable byte for byte; that identity supplies no runtime
qualification for the new lifetime probe.

No publication, ACTIVE/adopted playback, audio, IRQ, real musical timing, device
completion/voice-stop/quiet, physical or listening acceptance follows. Ordinary
TASK identity, placement and cleanup still require the separate exact-candidate
native run and its retained ownership boundaries. Aggregate stack, callback
depth and WCET remain separately unqualified. The accepted
exact musical schedule and all existing controller/provider inhibitors remain.
See [HOST regression and compile boundaries](../evidence/enhanced-editor/prepared-paula-lifetime-native-ram/host-and-guards.json), [native build scope](../evidence/enhanced-editor/prepared-paula-lifetime-native-ram/native-probe-build.json) and [default byte identity](../evidence/enhanced-editor/prepared-paula-lifetime-native-ram/native-default-build.json).


## Exclusive HOST editor mutation barrier —17 October2026

The separate explicit `PT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER=1`
compositor constructs its own genuine three-source owner and unpublished
lifetime for one existing editor and dedicated Paula bridge. It installs only
the existing exclusive veto hook. Cooperating edits and disposal are refused
while attached; the hook merely closes admission and requests cancellation.
It performs no allocation, child operation, stop, polling or backend call.

Later serialized task steps perform original captured cancellation, actual
local command/reader consumption, deferred resource drain, queue closure and
dedicated cache closure. Each step calls at most one original child lifecycle
operation. The existing fixed32-slot bridge close may release several blocks;
this is not a one-allocator-call or execution-time guarantee. Positive child
transfers and closures remain recorded before outer postchecks. Uncertain
ownership retains complete original contexts, without retry, reset, fabricated
quiet flags or restoration of a former sample generation. Newly allocated
cache intervals are checked against full declared control/context extents,
including tails, before later copy or release.

CLEANED still vetoes edits. A separate final detach requires positive original
closure and the exact original hook/context before removing that hook. The
fixture then makes child and barrier storage inaccessible before a successful
real editor edit/dispose, checking that no stale borrowed anchor is followed.

Two separate strict sanitizer executions pass the same13-case oracle: seven
complete cleanups and six retained fault contexts. One uses the preserved
working editor C/H pair; the other copies85 source bodies and replaces only
its private editor C/H with verified committed bodies (83 working bodies plus
two Git bodies). Neither changes canonical editor files or claims ABI equality
between those pairs. Both compile34 translation units with enabled assertions,
ASan/UBSan and strict warnings, and produce the sole224-byte marker with empty
compiler/fixture diagnostics. Tests exercise six genuine cancellation cuts,
actual edit/dispose refusal, callback reentry, full-context-tail allocation
aliases and a foreign hook. Genuine mono8/16/24 master, marker, cache and exact
in-memory project-save bytes are preserved; protected display work and the
classic baseline remain intact.

Nine separate object-only eligibility checks pass: one HOST positive, six HOST
refusals and two refusals from the genuine Amiga cross-compiler. No object is
executed by those checks. The initial compile-only missing declaration and a
separate clean-export Git-environment refusal are preserved; their successors
add only the required PCM declaration include and private temporary-directory
setting respectively. No fixture ran in either failed preparation.

The option is absent by default and requires the original HOST lifetime option.
The HOST option rejects native selection, including use with the separate
lifetime TASK RAM probe. The distinct mutation TASK partition follows below. This is
not publication, ACTIVE playback, native frontend/ABI/IRQ, output, device quiet,
timing, physical or listening acceptance. Direct editor init, preview-source
loading, workflow preparation and direct sampler/document writes are excluded
while attached: the existing hook does not universally intercept them. Native
New/Load must retain original storage until checked owner retirement, and final
disposal must honor refusal, before integration. Complete context lengths and
allocator freshness remain caller contracts. See [scoped HOST evidence](../evidence/enhanced-editor/prepared-paula-mutation-barrier/host.json).


## Native TASK mutation RAM partition —17 October2026

The separate value1 `PT_PREPARED_PAULA_MUTATION_NATIVE_TASK_PROBE_ONLY` mode
requires the original native lifetime TASK value1, a genuine m68k/AmigaOS target
and neither HOST option. It compiles the same unpublished compositor C body;
source/lifetime primitives and the earlier native21 probe remain unchanged.
Existing native publication/collection inhibitors remain. There is no native
frontend, output provider or interrupt wiring in this partition.

The expanded eligibility matrix passes18 object-only checks: three positives
(HOST header, native header and native common compositor) and15 meaningful
refusals. The old nine cases and diagnostics remain. Forty literal project
headers are pinned for the common-object check, which uses the preserved
working editor header and supplies no link or committed-editor ABI proof.
The later complete build instead uses both verified committed editor bodies.

That separate35-unit native build now passes. Its92-file export contains89
committed source/recipe bodies plus three reviewed overlays: the new header,
probe and builder. Actual dependency queries and preprocessing select89 project
bodies and50 SDK headers. Eight newly observed standard-library headers were
reviewed and explicitly pinned after discovery; no SDK root or unrelated input
was automatically admitted. Seven pinned runtime inputs and35 objects produce
42 exact link-map LOADs. Enabled assertions, original compiler safety flags,
raw stack-usage files, HUNK inspection and final input pins pass. The executable
is242,936 bytes, SHA256
`ff636f0ff1618326d89f99c827ea537f81c4915e37930ad254184fbec7bf5f8b`,
CRC32 `31D07C35`. It has **NOT_RUN** in Amiberry or on the A1200.

The compact probe's planned runtime is one actual initialized editor lifecycle:
checked empty-owner allocator setup; genuine mono8/16/24 Fast masters and three
selective Chip caches; one unpublished enqueue; actual editor vetoes without
child work; later local stop and seven captured drain steps; queue/cache close;
and separate detach before successful real edit/disposal. Complete static
contexts require actual Fast endpoint observations; persistent allocations use
the checked Fast registry. It observes real void-initializer results before
acquiring resources, keeps the editor alive through all external version unpins,
checks exact master/save bytes and requires final zero explicit ledgers. Any
uncertainty retains the entire original process. These are unexecuted native
assertions, not established placement, cleanup or zero-backend-call facts.

Both current-header HOST sanitizer executions also pass their unchanged13-case,
seven-complete/six-retained oracle against the working and committed editor
pairs. Source and layout preservation remain separate from native execution.
Raw per-function stack records establish no aggregate stack, callback-depth or
execution-time bound; whole-library accounting and complete child-tool/SDK/dyld
provenance remain outside scope. The original exact musical schedule is unchanged.
No published/ACTIVE playback, native frontend New/Load/exit, output/IRQ/DMA,
device quiet, physical or listening acceptance follows. See [current partition
build and regression evidence](../evidence/enhanced-editor/prepared-paula-mutation-barrier/native-task.json).


## HOST published/adopted editor barrier —17 October2026

The explicit HOST compositor now publishes its genuine owned queue once and
collects separate typed command/reader cancellation attempts after the original
stop returns. The editor hook remains veto-only: it closes admission without
child/backend/resource work. Later steps use original captured drain, queue and
cache closure, followed by separate authenticated detach. Semantic terminal
`good0` can complete with sticky child `BACKEND`; uncertain custody stays retained
without retry or generation restoration. An outer compositor fault does not
invent a child failure classification.

Two separate strict sanitizer executions, using the working and committed editor
C/H pairs, each pass13 published cases (eight complete,five retained) and four
synthetic adopted-reader cases (three complete,one retained). Each checks34 real
editor veto groups, including six at retained boundaries, exact master/cache/save
gold, C-first reader access after command detach, R-first geometry removal and
stale captured cleanup without resetting generation. Complete child workspaces
become `PROT_NONE` before later observations/service/detach; a real edit/dispose
succeeds only after detach. The original13-case unpublished oracle also passes
unchanged for both editor pairs. These are separate HOST source-pair results,
not ABI equivalence.

The native35 rebuild passes and reproduces the entire242,936-byte `ff636f0f` /
`31D07C35` HUNK exactly. Its strict raw-object comparison **refused**: all35 raw
objects differ. A separate investigation confines each difference to the first
`HUNK_UNIT` output-name/length/padding record (+8 bytes); remaining record streams
and code match. This does not establish raw-object identity. All17 HOST adapter
identifiers remain absent across35 saved native preprocessing units. The new
HOST publication/collection APIs remain excluded natively; mutation-probe
Amiberry/hardware execution, frontend New/Load/teardown, IRQ/output/device quiet,
real timing and listening are still unqualified. See [scoped HOST and native
partition evidence](../evidence/enhanced-editor/prepared-paula-mutation-barrier/published-host.json).


## HOST donor-preview dispatch veto —17 October2026

The MOD/PP20 file dispatcher now calls the existing editor change hook before
marking or loading a donor preview. This closes the missing entry check beside
the existing RAW/WAV/SVX checks. Format-sniff reads remain permitted. Direct
`pt_editor_source_load` and `pt_editor_source_load_with` APIs remain outside this
entry guard; this is not protection for every frontend or mutation route.

A genuine HOST fixture owns three 8/16/24-bit masters, three selective caches
and an unpublished command/reader queue. All four MOD/PP20 × `source_only=0/1`
calls refuse while the custodian is attached, preserving the existing donor,
preview flag, source generation, sampler budget, master/save bytes and caches.
Authentic later cleanup and separate detach permit four actual bounded donor
loads; final ledgers are zero and no backend was called. The working-editor
run passed with 38 translation units, 93 literal project inputs, assertions,
ASan/UBSan, empty compiler/fixture diagnostics and the exact 209-byte oracle.
The original sample-dispatch HOST regression also passed under its unchanged
legacy recipe. Its recipe has no explicit assertion-enable flag or per-child
120/30-second limits; those stricter controls apply to the new preview fixture,
clean-pair run and baseline control. Working temporary export/donor bodies were
removed by the original wrapper; retained pins agree with saved clean/baseline
bodies, without rereading the missing working export.

An independent negative control used the actual committed dispatcher body with
only this new line absent. The same fixture compiled, then stopped at its first
MOD/`source_only=0` refusal assertion with the expected SIGABRT; all other 92
copied source bodies matched. This establishes that the regression detects the
missing guard, not that the baseline completed cleanup. The earlier missing-
header compile failure and pre-test import-cache stall are preserved separately.

A separate complete committed-editor C/H run also passed the same 38-unit,
93-input fixture and 209-byte oracle, using 91 working input bodies plus the two
verified committed editor bodies. Compiler/fixture diagnostics were empty and
source, donor, preservation and control checks passed. These are separate HOST
qualifications; they do not establish ABI equivalence between editor pairs.

No new native build or execution qualifies this guard; the existing native35
artifact excludes the platform dispatcher. Amiberry, real-A1200, whole-application/
frontend integration, device/output/timing and listening acceptance remain
untested for this change. Independent saved-data review passes; see [scoped evidence](../evidence/enhanced-editor/prepared-paula-mutation-barrier/source-preview-host.json).

### HOST stopped workflow admission — 17 October 2026

Stopped Cleanup and Copy Range requests now consult the existing editor change
hook before setting `pending_apply`. A refusing custodian can therefore complete
its later authentic cleanup without an unstarted workflow request. Public Apply
keeps its original active/hook checks; active requests keep their existing pending
and explicit Stop+Apply behavior. No API, layout, receipt or native grant is added.

The new HOST fixture passes two independent genuine-owner cases. Each completes
a real usage preview before acquiring three 8/16/24-bit master/cache dependencies
and an unpublished command/reader queue. Actual stopped UI requests refuse with
pending, jobs and owned resources preserved. Later top-level stop/drain/close and
separate detach permit a real Cleanup or Copy transaction and one undo; exact
master/project-save gold returns and final ledgers are zero, without backend work.
The working-editor run passed 34 translation units/85 literal inputs with the sole
205-byte oracle, assertions, ASan/UBSan and empty compiler/fixture diagnostics.

The unchanged workflow HOST regression separately passed its silent navigation,
active pending/Stop+Apply, stale-range, cancellation, exact-copy/undo, event-origin
and summary-reuse assertions under its existing legacy recipe. This compatibility
result is distinct from the new stopped-custody cases and from physical playback.
Workflow idle is not serviced during prepared custody by this new fixture.

The independent old-workflow control compiled the same fixture with only the
actual committed prechange workflow body substituted. It aborts at the first
Cleanup request's exact pending/job-zero assertion, as expected; the other 84
source bodies and fixed fixture/wrapper pins are unchanged. Empty compile
output and no sanitizer error accompany this negative witness. It does not
complete cleanup or qualify the baseline as a passing implementation.

The separate complete committed-editor C/H run also passes the same two cases,
34 translation units/85 inputs and exact 205-byte oracle, with empty compiler/
fixture diagnostics and all source/export/admission/control/preservation checks
passing. It uses 83 working bodies plus the two verified committed editor bodies;
these separate HOST passes do not establish editor-pair ABI equivalence.

The changed workflow belongs to the old native35 compiled closure, so that artifact and
source-bound packet are externally superseded for current source; no old native
qualification transfers. A new native build/runtime, frontend coexistence, output/
device quiet/exact timing, Amiberry, real-A1200 and listening acceptance remain
unqualified.

See [compact HOST evidence](../evidence/enhanced-editor/prepared-paula-mutation-barrier/workflow-host.json).
