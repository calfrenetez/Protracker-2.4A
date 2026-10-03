# Persistent readers and recyclable commands

`scheduled_readers.c/.h` is a new opt-in portable contract. It preserves the
existing scheduled flags7, `scheduled_lineage` v1, and `ALL_RETIRED` semantics.
It does not adapt or cast their exclusive owners. Legacy immediate output and
native Paula/AmiGUS paths do not implement this contract.

The queue has separate fixed command and persistent-reader limits (1–8 each),
with up to four actions in a command. Open allocates one bounded queue. Enqueue
copies declaration metadata, at most four sets of128 spans, before any owner
callback. There is no further core allocation, conversion or cache acquisition.
Callbacks and queue operations are serialized task work. Current/status callbacks
must not edit borrowed declarations or release holders. Terminal callbacks must
not release their holders themselves; the designated release callback runs once
after terminal classification. All callbacks forbid reentry or queue changes.
Reentry latches failure and retains uncertain submitted ownership.

Each TRIGGER transfers a genuine independent persistent-reader holder plus a
separate small command holder. Sample/cache full-capacity spans belong solely
to the reader domain; the command holder owns only its complete small mutable
control extent. All actual sample and loop geometry must be covered by reader
spans. Reader control, command control, and resource spans are disjoint; genuine
independent reader pins may share immutable sample/cache storage. CONTROL and
STOP transfer only a new small command holder and hold a counted reference to
a registered actual ACTIVE original reader.

The backend must explicitly declare split-domain ownership and conditional
whole-key publication/activation in addition to the original exact timestamped
atomic grid contract. Capability fields are declarations, never qualification.
It must compare every original key and actual held/current reference under its
own task/activation synchronization before any effect. Activation cannot allocate,
convert, acquire caches, or traverse editor owners. This portable seam supplies
neither IRQ exclusion nor a native reference-transfer implementation.

The original key contains queue/session/generation/original TRIGGER ticket,
reader token/monotone serial/action/slot. Its original frame/window and actual
observation/issue ticks remain in the persistent record after the small command
slot is recycled. Reader lookup uses the original trigger/action, not a recycled
command pointer or a later CONTROL ticket. Sessions are nonzero caller-issued
identities and must never be reused across queue lifetimes, including allocator
address reuse. The bounded queue cannot detect global caller session reuse.

| Actual evidence or action | Command domain | Persistent reader domain |
| --- | --- | --- |
| Local, never-submitted cancellation | Release that command | Release only its locally reserved TRIGGER holder; CONTROL does not retire its source |
| Accepted/pending or uncertain submission | Retain | Retain; acceptance/issue/cache HIT is not adoption |
| Positive ADOPTED + ACTIVE original TRIGGER | Retain until detach | Expose original reader key after current check |
| Exact COMMAND_DETACHED | Release/recycle only the named event and full small holder | Retain, including unadopted/unknown reader storage |
| Exact READER_RETIRED with referencing commands | Retain referencing commands | Retain until their independent detach proofs |
| Exact READER_RETIRED with zero referencing commands | Already detached | Release exactly once |
| STOP admission or positive draining | Retain until detach | Close later key admission, retain reader resources |
| Positive actual same-slot replacement | Independent command lifetime | Permanently close older key permission, including after replacement retirement |
| Unknown, partial, forged or wrong-domain evidence | Retain possible references | Retain possible references; no inferred reset or retry |

`COMMAND_DETACHED` binds the exact event address, command holder/context extent,
ticket/session/generation/token. It proves no future activation/callback/reader
references any **command-owned** byte. It is not legacy `ALL_RETIRED`, a generic
no-DMA assertion, or evidence that shared sample capacities are unused.
`READER_RETIRED` independently binds the exact persistent key/registered domain
and full holder extent and proves no future activation/reader/callback can use
that holder or its full sample/cache capacities. Reader release additionally
waits for zero referencing commands. The backend cannot derive either proof
from submit acceptance, issue classification, ordinary callback completion, or
cache-hit status.

Local cancellation reports explicit `LOCAL_UNSUBMITTED` origin, `CANCELLED_BEFORE`,
`STATE_UNKNOWN`, genuine original keys, zero ticks and NULL event/context. It
cannot be mistaken for actual backend detachment/adoption. Backend command
receipts must carry `BACKEND_ACTUAL`; a local-origin claim from a backend refuses.

Exact release envelopes remain independently useful after invalid classification:
the core latches failure, leaves the caller output unchanged, and releases only
the independently proven domain. Its terminal callback receives valid0. Reader terminal validity is monotone: a corrected duplicate cannot erase a
previous bad retirement classification while resources wait for command detachment. A
correctly bound detach with a malformed adoption classification does not invent
ACTIVE or release the reader. A correctly bound retirement with malformed timing
does not invent valid playback. Incorrect envelopes retain the domain.

STOP admission conservatively closes later descendants even if that STOP is
subsequently canceled. Explicit reader cancellation also closes admission without
fabricating DRAINING or retirement. Positive replacement closure is permanent
through either command or reader observations, even if the new reader is retired
before the old record is polled. Actual backend activation must repeat the full
conditional comparison; an earlier accepted command is not permission to affect
a replaced or stale source.

All public write outputs are checked before callbacks, release or publication
against the whole queue/backend mutable controls, every held command/reader
holder, and every held reader full resource capacity, including readers delayed
by command references. Enqueue also rejects new resource capacities intersecting
any live reader control. Span guards are metadata-only, use checked address
arithmetic, and never read unused sample padding. Aliased, pending, uncertain or
forged-output refusals preserve caller output. Stop closes publication and drops
only local entries; submitted domains require explicit bounded polls/cancels.
Close refuses while either domain remains held. No automatic reset/retry or
force-unpin exists.

The focused synthetic fixture exercises20 sequential CONTROLs on one retained
reader with command capacity2; both detach/retirement orders; unknown/unadopted
retention; command and reader pressure; stale receipts after slot/token reuse;
fresh-session queue-address reuse; ticket/serial rollover; all key fields;
malformed independently bound domain proofs; same-cache replacement through
command and reader receipts; STOP/cancel behavior; complete output/padding/control
before-images; reentry; actual whole-batch/current and clock-window refusal, four original grid
configurations, unsupported capabilities with no allocation, and allocator-release
reentry refusal.
It includes the production C exactly once for private rollover checks. These
synthetic holders are portable ownership tests, not sampler/cache-pin integration,
Exec Fast/Chip allocation, native IRQ/reference-transfer, device, DMA, musical
timing, physical, playback or listening acceptance.

The frozen focused ASan/UBSan group passes with two successful compile/run
commands, two full dependency scans, 12 canonical and 99 system inputs, and one
archived executable. A separately reviewed pinned 68000/nix20 portability build
passes with 13 compiler/toolchain commands, the same 12 canonical inputs, 30 SDK
inputs and seven runtimes. Its 61,416-byte `PTScheduledReadersTest` HUNK retains
the original enabled SDK assertions and one exact fixture marker. It has not run
on a native target; stack adequacy and actual backend adoption/detachment/retirement
remain unqualified. See [host and compiler evidence](../evidence/enhanced-editor/scheduled-readers/README.md).
