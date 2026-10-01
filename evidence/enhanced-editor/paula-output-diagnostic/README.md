# One-slot start diagnostic: observation and closure PASS

Candidate tree3daae9cc97a81d0a2f92de689f4405ae7833c7ab. Single silent slot0
WRITE, owned zero32-byte Chip buffer, period400/volume0; original start result0
preserved. Immediate: notified0, held1, pending1, error0, flags144, unit1,
request incomplete, DMA1. After one tick: notification drained1, original result0,
held1/pending1/error0/DMA1. This run proves a late notification while DMA was
already active; prior failed RC14 remains failed with its cause unlogged.

Explicit bounded request/control/FREE/LOCK/channel/device/port closure and Chip
release, exact harness cleanup and independent subsequent locked running/DMAoff/
pathabsence PASS. AmiConnect RELEASE acknowledged; no recovery holds. No controls,
other three voice starts, audible playback, timing, UI, lifecycle or physical tests.
Binary17376bytes SHA256ccbe52f58f1ea283ae5abcd95b20c7910bcf3fd8fc1d856a869dfed3adcf0a6b.
All92 source hashes matched indexed tree; guard built, not executed.
