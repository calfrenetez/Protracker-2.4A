# Mixed shared-sequence interval and forecast qualification

30 September 2026. Injected software and native allocation fixture only; no real
MMIO/device/DMA/audio/clock/physical or full native PLAY acceptance.

The combined owner now drives ONE retained audited shared sequence. Next publishes
an interval, consume advances1..256 caller-declared elapsed frames, prefetch copies
that pending interval and advances only copied phase <=256 frames or one existing
cache/conversion step. Both backend routes must be ready. Complete requires all
live frames consumed, performs no synchronous preparation/allocation/upload/command
conversion, commits copied sequence state once and emits original global order.
DONE retains readers/pins until close. Faults poison/cancel the sequence and attempt
retained stops once if captured API identities remain safe. No clock scheduling yet.

Expanded ASan/UBSan host fixture PASS7.616s,29 modes at8/16/24bits. New cases cover
44.1/48kHz full shared traversal with unselected-track tempo change and measured
frames/interval parity, partially consumed forecast/live consume interleave,
early/unready completion refusal and unchanged interval outputs, bounded cache
steps with no voice starts, write/allocation-free completion, partial and ready
cancellation, DONE pins through failed barriers, active-reader stale and late
cache refusal, failed start and uncertain-stop retention. Earlier commit/staging/
master ownership tests retained. Existing underlying modules were unchanged.

Selected portable/native mixed-owner targets+guard PASS160 indexed sources,
zero generated,3verified binaries. Exact source tree and hashes are recorded.
Shared030 ONE normal fixture PASS49.490s:930 actual Fast allocations, zero owned
bytes, actual Chip allocation/release assertions. Bounds200s overall/140s runner/
90s fixture; reservation<=240s. Fresh AmiConnect no-conflict/no-hold and idle peers,
standard locked live guards and independent subsequent locked running/all4DMAoff/
owned run+launcher absence PASS. Explicit RELEASE sent after verified cleanup.
No guest retry/reset/resume. Qualification script expects its original
build/dev/run-mixed-sequence-qualification.py location. No binaries tracked.
