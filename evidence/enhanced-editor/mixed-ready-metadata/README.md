# Share Paula bridge metadata within one prepared-ready check

1 October 2026. `pt_paula_prepared_ready_owned` already validates all captured
source/voice/API/context/map identities and calls `current()`, which validates
Paula bridge metadata. It now uses the private validated-location helper for
active/candidate leases in that same serialized call. No callbacks or reentry
occur between that metadata validation and these lookups. Validation is not
retained across calls. The standalone prepared-location API still performs the
full metadata check on every call.

Every route, lease slot/serial/pins/data, valid/version and returned byte-count
check remains; the ready checker still validates source descriptors, playback
pointer, alignment, offset-plus-length bounds and word count. AmiGUS callbacks,
union master/source checks, immutable-reader checks and deadlines are unchanged.
The helper contract explicitly forbids arbitrary inputs, mutable metadata,
callbacks/reentry and carrying validation to another public call. Its only
production call sites are the fully validating location wrapper and the two
loops inside the fully validating prepared-ready checker.

Four related ASan/UBSan host modules pass in 24.026 seconds: Paula dispatch,
selected cache/job ownership, full Paula song, and mixed owner (201 scenarios).
The three new precision cases each prepare a live Paula reader and two genuine
candidate master/cache leases, then reject 22 between-call adversarial changes:
bridge generation/table/count/channels/routes/closing/version/project, sampler
generation, master source storage, captured source storage, candidate serial/
valid/version/storage, live-reader serial/uncertain/track, playback pointer/words/
range and project tempo. After each restoration, readiness twice succeeds;
cache-pin/output/allocation/upload counts are unchanged. Stale standalone
lookups preserve both output pointers/counts. Cancellation releases candidates
while preserving the live reader; owner close subsequently drains it.
The initial test incorrectly wrote zero over an already-zero generation; this
no-op was corrected to a genuinely different generation before qualification.

Indexed tree `7762368aa08a17bfc334284f82bbff8847e7afc8`: 162 source hashes verified
against candidate and tree, zero generated sources, three verified binaries.
PTExecMixedOwnerTest 243168 bytes, SHA-256
`b6ca1ea991a103f94a0a1b8476fb2b8e82a3b4e879d1c9c0d307c1e8de834c7e`.

ONE shared030 native run `1790837289850683000` passes 18 cases (prior 15 plus three
ready-safety cases) and 120 separate accessor observations in 17.606 seconds
overall. Bounds remain 90 seconds fixture / 140 runner / 200 orchestration under
the coordinated 240-second reservation, including cleanup. Normal done/rc0,
207 Fast allocations with zero owned bytes, Chip release, all pins/readers/leases,
workspace/private unsubmitted E-clock/device/request closure pass. Exact cleanup
and independent subsequent locked process/profile/bridge/68030/running/all-four-
DMA-off/run-and-launcher absence pass. Explicit RELEASE acknowledged by AmiConnect
and forwarded to peers. No retry, lifecycle, alarms, output MMIO/DMA/audio,
screenshots, clock changes or physical operations.

All raw observations and outliers are retained. Component complete: two voices
2.385–2.508 ms; sixteen voices 5.024–25.333 ms (large outlier retained). Running
service: two voices 1.183–1.545 ms; sixteen 2.419–3.379 ms. Running boundaries:
two voices 2.794–23.208 ms; sixteen 4.879–6.313 ms. Counters use 709379 Hz.
Accessor observations continue to exercise the standalone fully validating API,
so they do not measure the new private helper alone. The structural reduction
in metadata checks is established; these noisy instrumented samples do not prove
an exact speedup, actual-time feasibility or audible playback. At 48 kHz a
128-frame span is 2.667 ms; excess costs and outliers still need investigation.

Next inspect repeated genuine source-pin validation for duplicate Paula triggers
within one immutable plan. Share checks only for the exact same sample/current
pin within one callback-free serialized call, retaining each action's descriptor
and lease/address/range checks. Do not persist validation across calls or weaken
stale-reader ownership. Native PLAY/event-loop/device/IRQ integration and hardware/
audio acceptance remain open. Physical hardware stays off and unprobed.
