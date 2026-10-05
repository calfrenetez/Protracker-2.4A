# Changed progress candidate — first startup failure preserved

The exact changed candidate from commit120e51d is PTMasterProgressTest292912B,
SHA2562c88205c370a3ab8d68a0f5beecc851d0d9e44e9e90a95d590a74185d03348e8.
Its host sanitizer/progress/parser tests and pinned compile passed separately.
Fresh AmiConnect ACK20 covered one exact030 software window and completed-only
settlement. EMPTY34/no-emulator/disconnected endpoint/locks/pins/product guards
passed. One reservation and one existing protected wrapper startup occurred.

The wrapper failed before staging or launching the fixture. Its captured nested
trace ends in GuardRefused: Target tool reported an error, from amiga_connect.
The guard retained HOLD40, no inflight, potentially resident root reservation.
The underlying error text was not captured by the shared rejection helper, so
this is not a diagnosed product bug or hardware fault. Host-only source inspection
confirmed one amiga_connect call and no retry loop; multiple nested HOLD entries
do not establish multiple exchanges. No unchanged startup replay occurred.

Host-only process and saved runtime identity confirmed root-owned PID35970 and
its exact030 profile/command. The candidate namespace is absent; run_once was
never invoked. No post-failure target/service request, signal, cleanup, disconnect,
reset, service restart, guard release or physical action occurred. The failed
startup emulator remains held; the coordinated window is NOT released.

Concrete separate recovery is prepared and root source/AST reviewed: fresh three
locks, exact PID/runtime/profile/private lease/full guard/input checks; one SIGTERM
with five-second bound; no forced kill; preserve all first records; require
emulator/listener absence and original healthy disconnected endpoint; separate
independent readback and four-record normal release. Direct human scoped approval
is pending. Prepared scripts refuse without it and have NEVER executed. The
question exists because this window explicitly excluded failed-target cleanup.

Private lease and compiled product are excluded. Recovery reviews are root
self-reviews, not independent agent review. Native candidate execution, memory
placement, aggregate stack, timing/IRQ/device/audio/listening and physical tests
remain NOT_RUN. Prior60s fixture failure and its successful recovery remain exact.
Run verify_saved.py only for saved bytes/facts; copied controls are historical,
not current target entry points. Future target actions require fresh coordination.
