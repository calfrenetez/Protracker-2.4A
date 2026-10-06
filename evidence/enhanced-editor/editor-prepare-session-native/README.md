# Exact preparation-session emulator qualification — 6 October 2026

The full `PTEditorPrepareSessionTest` from source commit
`86cb65fdef27a19082a4d469acee8aa3bb39e0bb` passed once in the shared
68030/no-FPU/AGA emulator with 2 MiB Chip and 128 MiB Fast RAM configured.
The 395508-byte executable SHA256 is
`b9b9a71351a012f20b5dc67228604f2f6e02d7dce93d07b2fed28fcb8cf83d10`.
All three complete stdout markers matched the 692-byte host result, RC0 and
COMPLETE, observed after 343.511 seconds under the unchanged 420-second bound.

Executed portable assertions cover 15 existing editor regressions, 39 checked
lifetime cases, 18 checked admission aliases, 102 preparation-session lifetimes,
78 session admissions and three expired-terminal cases across 8/16/24-bit masters.
Clocks, voices, output callbacks and sample bus are injected ordinary software.
The complete application, real Paula/AmiGUS output, native IRQs and musical
activation timing were not tested. Stack65536 is configured, not measured.
Physical execution of this exact coordinator remains NOT_RUN. Separate later
Paula reader controller work is outside this candidate.

The first v1 host promotion refused nonresident installed Python interfaces.
Those ordinary files were hydrated without target access. The v2 substring
check then refused `emulator_main`; a separate AST check resolved that host-only
diagnosis. Independent v2 review found two real caller gaps: failure before
private lease serialization and lack of immutable product checks after copying.
The distinct v3 caller uses the actual local reservation on startup failure and
binds in-memory/staged/run bytes to the promoted fingerprint. All 27 isolated
failure cases passed before independent review and root preexecution review.
Original refusal and blocker records remain preserved.

Fresh affirmative hands-off receipts from all three peers, including Scott's
explicit priority deferral, preceded fresh live guard admission. One reservation,
one startup and one candidate launch were used. Completed-only settlement moved
the six exact known files into retained custody, checked guest task absence,
original CPU/RAM/AGA and disabled audio DMA, saved the OS screenshot, disconnected
DevBench and normally closed our owned emulator. No force kill, deletion, retry,
physical action or shared harness alteration occurred.

Separate independent release verified process/listener absence, healthy
disconnected original127.0.0.1:2345, the unchanged bridge flag and all2386 pins,
exact custody and four release records. It returned EMPTY108 with no reservation,
inflight operation, recovery hold or possible resident. All three peers were
explicitly notified. This records the completed window; it is not continuing
ownership or permanent availability.

Run `python3 -B verify_saved.py` for saved byte and relationship verification.
It performs no target query, producer replay or reservation. Operational callers
are custody records referring to their original private window and must not be
replayed from this packet. The executable, private lease and raw private release
archive are excluded. A metadata receipt records root's archive byte readback;
the public verifier checks that receipt against the released-state fingerprint.
