# Audio.device voice prototype: FAILED native qualification

1 October 2026. Indexed tree `f427cfadb226cd8693e3f319f0979144ae32a1f6`;
92 compiled source hashes checked against indexed export and Git tree, zero
generated sources. PTExecPaulaOutputTest 17032 bytes SHA256
`38fe2dfa97a001cc34d4df3fa67b2fbe4d605ad66818ab33160be7225755ab51`.
Guard binary built but not executed. This is not a native output pass.

The host-tested adapter reserves channels through the separately qualified
reservation owner. It allocates eight fixed write/control requests, a private
start-message port and the reservation completion port. PAL/NTSC capability
uses OS2+ documented Exec E-clock frequency. Signed8 data must be caller-owned,
contiguous, immutable, aligned Chip storage. Start success requires an actual
write-start message; submission alone never succeeds. Pending/error start retains
data for stop. Control requires completion. Stop aborts each WRITE once, checks
completion before WaitIO, drains start messages and retains storage/control
contexts until all requests complete. Quiescence precedes reservation/free/lock
closure and request/port disposal. No custom interrupt handlers or MMIO writes.

Primary contracts: [audio CMD_WRITE autodoc](https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node04B6.html),
[Audio Device](https://wiki.amigaos.net/wiki/Audio_Device),
[Exec E-clock field](https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node007C.html).
These describe start notifications, BeginIO flags, allocation and readable clock
metadata. Device start acknowledgement may be asynchronous; the current voice
owner treats non-immediate success as uncertain and stops rather than upgrading.

ASan/UBSan host21 output cases PASS0.602s and host12 reservation regressions
PASS0.415s. Real header with stubbed Exec/device: setup failures at13 resource
steps, clock refusal, four starts/controls/stops, geometry/Chip checks, delayed
start and stop/late notification, delayed control retention, device start/control
errors and clock changes. Existing routed reader/confirmed-release regression
also passes. Host results do not establish native start semantics.

ONE coordinated shared030 run `1790842848954463000` FAILED with normal RC14,
meaning at least one start was unconfirmed. The fixture log is empty: exact
reason (missing notification, device error or validation) was not recorded.
Do not present asynchronous acknowledgement as the proven cause. No retry,
reset, resume, lifecycle change, audible/listening or physical operations.

RC14 can be returned only AFTER bounded adapter close succeeds (else17),
all4DMAoff/private port absence checks succeed (else18), and FreeMem32 completes.
Thus the exact candidate's RC14 path proves owned WRITE AbortIO/completion,
control completion, FREE/LOCK completion, requests/ports/device closure and Chip
release. Harness records finished normal termination, failed qualification,
exact cleanup and DMAoff. Independent subsequent shared-locked process/profile/
bridge/68030/running/all4DMAoff/run+launcher absence check PASS; read-only only,
no cleanup mutation or new launch. Temporary hold released/acknowledged by
AmiConnect and forwarded to peers. No current resource hold remains.

The native silent volume0/zero32byte fixture attempted actual WRITE/DMA, with
read-only DMACONR observation planned after all starts. It failed before that
all-channel assertion and before controls. Four native starts/controls are NOT
qualified. This adapter is not installed into mixed frontend or classic player.
Next: instrument exact refusal/state and evaluate honest start lowering against
the synchronous voice API; new immutable diagnostic bytes and fresh coordination
are required before another guest window. Preserve this failed result.
