# Public readers backend registration bindings

This change applies only to the opt-in `scheduled_readers` descriptor ABI and
increments `PT_READERS_VERSION` from 1 to 2. A backend declaring numeric version1
is refused before queue allocation or backend callbacks. Original scheduled
flags7, lineage-v1 and reference-flag values remain unchanged. Five compatibility
fixtures replace seven positive numeric constructors with `PT_READERS_VERSION`;
these are intentional ABI updates, not changes to their ownership contracts.

A published `pt_readers_event` carries `binding={context,context_bytes}` for its
complete command holder. Each immutable `pt_readers_domain` separately carries
the binding of its independently held persistent-reader control. Core copies
these opaque identities from the genuine declaration metadata already captured
before callbacks. It retains no caller control declaration and adds no callback
or allocation. Backends may copy the identity values into receipts; they must
never dereference, copy, mutate or release owner storage or invoke its callbacks.
No owner current/release/terminal function is exposed. Declared reader resource
spans remain separate from control identities.

Receipts bind the full original queue/session/generation/ticket/token/key,
registered descriptor address and opaque context/extent. Existing private-holder
envelope checks remain authoritative: forged bindings cannot authorize release
even if the other fields match. Before reporting `COMMAND_DETACHED`, the backend
must drop all references to command-owned storage. Before reporting
`READER_RETIRED`, it must drop every persistent reference to that exact reader
holder and its sample/cache capacities. Core reader storage may remain owned
until both independently exact retirement and zero referencing commands; that
delayed disposal never extends backend borrowing. Remaining command metadata
cannot revive the retired reader or inspect a recycled command as its old ticket.
Command detachment and reader retirement are independent obligations.

Bindings alone prove neither currentness nor ACTIVE, ADOPTED, command detachment,
reader retirement, upload completion or voice-stop. Existing task-side checks and
the backend's atomic actual-key/current-reference validation at publication and
activation remain required. Activation cannot traverse owners or allocate/convert.
Original absolute frames and exact `ceil(frame*frequency/rate)` admission windows
remain unchanged. There is no immediate-start, predicted-timestamp or rebase
fallback.

The public-header-only fixture separately compiles core queue, genuine reader
adapter and song producer. It exercises actual sampler-owned mono8/16/24 masters,
full-capacity source before-images, discarded caller scratch, genuine initial
setup/audit/same sequence/lookahead and selective cache leases with a synthetic
backend. It tests independent detach/retire orders and command reuse, numeric
version1 refusal, forged command/reader contexts and extents, uncertain accepted
submission, retained domains and correct proof-driven cleanup after sticky
failure. Receipts use public bindings only, with no private implementation C
inclusion, opaque-holder inspection/copy or exposed owner callbacks. Alias probes
check unchanged external receipt outputs and original source bytes plus separate
command-poll and reader-poll counters; clock-call counts alone are not polling
proof.

## Actual saved validation

Host and compiler results below bind completed actual receipts and saved bytes.
Native execution remains separate. Prior saved-reader refusals and classifications
are retained; neither the passed host run nor the compiler build was repeated.

| Scope | Status and required receipt binding |
| --- | --- |
| Seven affected ASan/UBSan host groups | PASS: 7 groups, 14 compile/run commands all RC0, 172 full-M queries, 84 canonical inputs, 105 external SDK inputs and 7 retained products. Complete source inventory has 8,137 files before/after; actual host duration 59.471607 s. |
| Public-fixture pinned 68k compile/link | PASS: first/once 42 RC0 commands, 31 ordered full-M queries, 75 canonical and 32 SDK dependencies, seven pinned runtimes, enabled SDK assertions and one complete CPP/HUNK registration marker. HUNK216820 B SHA25672b40262c3c6c54351f7b1fe94042d8672f4a8a4a99e19ed99d60402bfebbf9f. Ordinary malloc entry; no Exec wrapper or execution. |
| ABI2 native/emulator/physical execution | NOT_RUN for this registration scope; compiler or host results do not change this status. Total native stack remains UNKNOWN. |

The host record is `host-checks.json` SHA256 `0dbd26c3e127a84c9a5b1bd3577c3da602c640dcdd881c7f9fc4ec7c540c51f9`;
root's corrected saved-byte review is `root-saved-host-review-v2.json` SHA256
`848b2dbd4734308bf4f514971e4db11206374a09013a07637559dc45ffb8af62`. The original reader's schema refusal was
host-audit-only; the actual seven-group run was not repeated. All 195 host durable
copy pairs preserve the original logs, dependency records and products. The new
public fixture emitted all three mono8/16/24 success lines at 212 steps each and
its full registration footer after the old-version and five forged/uncertain
cases. These are synthetic software ownership results, not target timing or
memory-placement observations. The compiler record and its 45 original/durable
copy pairs retain the separate portable HUNK; no native product was executed.
See [saved evidence](../evidence/enhanced-editor/readers-backend-registration/README.md).

The preserved compact protocol1 compiler evidence remains evidence for its exact
old frozen source and contract. It is not silently eligible for these ABI2 public
descriptors. Rebuilt compatibility fixtures need their own matching source and
runtime bindings; updating a source macro does not requalify an old product.

No native memory placement, backend activation, IRQ exclusion, DMA, AmiGUS card
capacity/order/completion, actual output timing, audio or listening qualification
follows from this change. Total native stack remains separately unqualified.
This Paula signed8/four-slot descriptor does not represent AmiGUS card addresses,
16-bit representations or rate/gain controls; a mixed backend ABI is separate.
