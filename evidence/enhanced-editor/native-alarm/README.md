# Native absolute alarm ownership — 27 September 2026

Separate UNIT_WAITECLOCK owner: absolute deadline, no pending reuse/free,
CheckIO-gated WaitIO, at-most-once AbortIO, pending close retains all resources,
error poison and close/reopen. Host fake Exec ASan/UBSan2tests PASS0.436s, including
delayed abort and natural completion racing abort, partial allocation/version/open
failures, late/busy refusal, absolute hi/lo packing and exact release ordering.

Native build/main syntax PASS;170staged inputs/font and binary hash verified.
Binary396000bytes SHA256
e4500ef4d221b4d5957e42e492e8a7ccae2623a85925aa2971d1eeaf1a4fcd2c.
Shared030 render-files-1790505616172013000 passed within90s RC0. Four2ms absolute
alarms completed/replies collected,10second future alarm cancelled and resources
closed, reopen/close passed.1182Fast allocations/zeroowned/noChipfallback. All4
DMAoff/exactrun and launcher cleanup independently confirmed; explicitly released.

Observed lateness at709379Hz:13265,26212,26664,25941ticks. These include explicit
Delay(1) caller polling and read overhead. They are NOT intrinsic timer precision,
sustainable playback timing or physical evidence. This coarse poll path is not
connected to the strict song deadline gate. Native/card audio output disabled;
no physical, lifecycle/config or unrelated dirty display changes. The separate
EClock reader request remains unsubmitted; only the new alarm owner queues IO.
