# Injected recording PCM and interrupt ownership

Host: six ASan/UBSan cases pass in11.313s. An initial invocation additionally named
nonexistent test_amigus_interrupt_owner; its six real cases passed, and the corrected
invocation uses test_amigus_reservation (which includes that interrupt fixture).
No product assertion failed.

Shared030: run1790568106582275000 PASS/RC0,35 Fast allocations, zero owned bytes.
The tests retain capture staging and PCM access until both input stop and the
separate interrupt guard are quiescent. Pending/error/unknown acknowledgments,
busy/incorrect blocks, allocation and format refusals, cancellation and stopped
master transfer are covered. The caller still owns library/card close.

Source9650ba9d230a16b8b87db00c9f1ac4726929f6c6:119sources/3binaries hashes verified.
PTExecAmiGusCaptureTest120328bytes SHA256
7bdb737a868606acd7520d8b54309d2e0e642cd1a65f824669322a59ae842283.
Normal cleanup, independently locked running-guest/all4DMAoff/owned paths absent
and explicit AmiConnect RELEASE. No outstanding owner or hold.

The pinned SDK PCM flag includes recording/playback/mixer. This conservative owner
uses one exclusive access lease and does not provide duplex sharing. Callbacks and
card/interrupt objects are simulated; native input, card formats, register I/O,
recording quality and physical hardware are not qualified. The Amiga remained off.
