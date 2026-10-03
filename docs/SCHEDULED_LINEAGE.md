# Optional scheduled reader lineage contract

`scheduled_lineage` is a separate portable core contract. The existing
`scheduled_output` flags7 API and its implementation remain unchanged. This
module does not integrate the future sampler TRIGGER owner, CONTROL/STOP song
production, `pt_paula_prepared`, native output, interrupts, DMA or audio.device.
No actual hardware activation, timing, memory-class or listening capability is
implemented or qualified. Its backend declarations are promises to qualify.

Open requires the original timestamped/atomic-publication/reader-retirement
flags exactly7, plus exact lineage version1 and conditional-target flag1. Unknown
flags, versions, missing callbacks or invalid context extents refuse before
allocation. A fixed one-time controller allocation holds at most eight events,
each with at most four actions and one action per slot. It performs no conversion,
cache acquisition, eviction or sample reads. The caller prepares and independently
retains each event's complete master/cache/continuing-reader refs and metadata.
Public span guards inspect complete capacities without scanning their padding.
This first core fixture uses ordinary host allocation and synthetic held owners,
not sampler pins, MEMF_CHIP, actual voice readers or an installed native backend.

The caller supplies a nonzero never-reused session. This is a caller contract;
the core cannot enforce global uniqueness without an unbounded session registry.
Tickets and reserved reader serials increase without wrap/reuse within that
session. A reader identity is the complete queue/session/generation/original
TRIGGER ticket/action/slot/owner/serial tuple. A serial reservation, enqueue OK,
submit acceptance, a cache address/HIT or the absence of retirement cannot create
an ACTIVE key. The same numerical serial in a distinct never-reused session is
a different identity; old keys refuse even if allocator reuse gives a new queue
the same address. No handle/key may outlive its original queue.

Typed polling distinguishes pending, observation, all-reader retirement and
uncertainty. A genuine ACTIVE observation requires a complete correctly bound
receipt, an original TRIGGER, the exact reserved identity, actual command
observation/issue counters inside the unchanged original frame window, and an
ACTIVE reader state. Repeating that observation is idempotent only when immutable
identity/timing agrees. Moving backwards, changing a key/time, a partial command
batch, unknown values or contradictory trigger/STOP phases latch publication
failure. The command phase and reader phase are separate: an unissued CONTROL
can be cancelled while its positively activated source remains ACTIVE. Such
cancellation does not stop that reader or prove it never activated.

CONTROL/STOP targets are copied by value. They must name a registered positively
ACTIVE original trigger in the same queue/session/generation and stable slot,
with an earlier original frame and a current independently held source. A queued
or published intervening replacement before the intended frame refuses while
that entry exists. Positive actual issue of a replacement permanently supersedes
the old exact reader permission, even when the replacement ticket later retires
and its entry is cleared. This closes key lookup and publication without retiring
the old holder or inventing a reader-draining observation. A merely unsubmitted
or cancelled-before-issue trigger does not leave that permanent fact. STOP
admission conservatively closes subsequent descendants, including if the STOP
is later cancelled; this first contract does not restore a guessed lineage.
Repeated-slot actions and intra-batch lineage chaining refuse.

Before publication, the core rechecks every target/current owner and actual
counter/frequency against the original epoch/grid. A missed window or invalid
clock latches failure; it cannot be cleared by a later clock rewind. The backend
must independently check the complete target set at atomic publication AND at
the actual original activation window under its own synchronization. A task
snapshot is never a guarantee that a reader survives until that boundary. If any
target was cancelled, stopped, retired, replaced (even by the same cache) or
otherwise changed, the complete batch has zero effects. It may never update
whichever reader occupies the slot or use an immediate fallback. Activation is
backend-owned; the core implements no deadline/IRQ entry point and invokes no
allocator, converter, editor or owner traversal at activation.

Observations never release storage. STOP pending, STOP issued with draining
readers, and all-reader retirement are distinct. Each ticket independently
retains its own owner until an explicitly typed acknowledgement proves that no
future activation, DMA reader or callback references ANY event/owner storage,
including shared full cache capacities. The acknowledgement envelope must bind
the exact queue/session/generation/ticket/owner/action count; a mismatched claim
retains everything. With an independently correct retirement acknowledgement,
bad action/timing classification can latch failure while safely releasing once.
The optional terminal callback receives the final classification and its own
validity before exactly-once release; a previous queue failure remains separate.
Never-published local events may release on explicit cancel/stop since they have
no backend references. Local receipts explicitly carry LOCAL_UNSUBMITTED provenance and UNKNOWN
reader state; cancelling a local CONTROL cannot invent or revive an ACTIVE
source fact. Positively observed draining/retired facts through CONTROL/STOP
monotonically close the exact original reader key while preserving its original
command/timing and without retiring any other ticket. Only an original TRIGGER
ticket/action may expose a reader key; descendant tickets cannot bypass that
trigger holder's current check. Terminal callbacks may not reenter or free the holder themselves.

Uncertain submit/poll/cancel, callback reentry and failed/partial execution latch
further publication and retain possible readers. Stop permanently closes
publication, releases local events, and attempts at most one explicit cancellation
per submitted event per call. Pending/errors retain refs; close refuses while
held. There is no automatic retry, reset, force unpin, or destructor cancellation.
An explicit later poll/cancel can collect correctly bound retirement after failure.

The strict retirement rule has a finite-capacity consequence. A published
CONTROL sharing an active reader's cache cannot be released just because its
register command finished. Repeated controls can consume all eight retained
event slots until that reader retires. Capacity refusal is tested and preserved;
this is not a completed scalable song scheduler. Separating fixed command slots
from a separately bounded persistent-reader owner domain needs a later reviewed
reference-transfer/acknowledgement contract. It must not reinterpret flags7,
copy exclusive prepared owners or release a cache on acceptance/issue.

The sanitizer fixture checks actual-observation versus acceptance, cancellation
before/after activation, duplicate keys/times, same-cache replacement with complete
zero effects, source retirement before publication, STOP draining and independent
retirement order, unknown/forged/late/partial receipts, reentry, source/control/
backend/allocated-output aliases and wrapped metadata, same-address new-session
refusal, fixed-entry pressure and PAL/NTSC 44.1/48k frame-window parity. The
unchanged legacy scheduled-output fixture is a separate regression check. Host
checks qualify software behavior. A separate pinned standalone 68k compile/link
passed after one token-equivalent fixture newline correction; its 60,292-byte
HUNK has not run. The original compiler warning remains preserved. Native runtime,
adequate stack, IRQ, physical memory/output, timing and listening remain unqualified.
See [host and compiler evidence](../evidence/enhanced-editor/scheduled-lineage/README.md).
