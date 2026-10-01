# Deduplicate identical Paula master checks within one invocation

1 October 2026. `prepared_sources` now validates each exact sample/current-pin
pair once in each callback-free serialized invocation. A local 255-entry pointer
array is guarded by a freshly zeroed eight-word bitmap; entries are read only
when their bit has been set after successful validation. No array initialization,
allocation, callback or persistent validation cache. The bounded local workspace
is about 1 KiB on the native ABI. Sample bounds are checked before bitmap/array
access. A different expected pin for the same sample is always revalidated.
Master pins stay genuinely owned throughout the invocation; temporary retain/unpin
cannot release the owner's master. The bitmap is discarded on return.

All per-action source descriptor and playback lease/address/range checks remain.
This applies to incremental prepare/ready/apply source checking only. Mixed union
validation, standalone bridge metadata checks, backend ownership callbacks,
active-reader retention and strict deadlines are unchanged.

Four related sanitized host modules pass in 23.644 seconds: Paula dispatch,
selected cache/job, full Paula song and mixed owner (201 scenarios). The three
precision safety cases now prepare four candidates in nonconsecutive sample order
0,1,0,1 while retaining a live Paula reader. Each checks 26 between-call stale
mutations, restored/idempotent readiness, unchanged pin/output/allocation/upload
counts and candidate cancellation preserving that reader. Four added mutations
cover a duplicate's different genuine master pin, wrong sample index, descriptor
and lease serial. A same-sample earlier successful check cannot hide any of these
failures; a later invocation cannot reuse earlier validation. Full stale standalone
location outputs still remain unchanged.

Indexed tree `df7430a0d1d44b3982244f0d9e1becf1d9320f08`: all 162 source hashes verified
against candidate/tree, zero generated sources, three verified binaries.
PTExecMixedOwnerTest 243500 bytes, SHA-256
`e949760245c9cf3d8eeaa7d35c4f54b9ffcfa6aae4b6a35cc408374531940bb0`.

ONE shared030 run `1790837719570983000` PASS 17.890 seconds overall: native 18
cases plus 120 separate accessor observations (not 201 native cases). Six
component cases, six complete two-interval 2/16-voice running cases, three
cancel/reuse and three ready-safety cases. 207 native Fast allocations/zero owned
bytes, Chip release, every master/cache/readers/leases/workspace/private unsubmitted
E-clock/device/request closed. Normal done/rc0/exact run/launcher cleanup plus
independent subsequent locked process/profile/bridge/68030/running/all-four-DMA-
off/path absence pass. Explicit release acknowledged by AmiConnect and forwarded
to peers. Unchanged bounds: 90 seconds fixture, 140 runner, 200 orchestration,
within coordinated 240-second reservation including cleanup. No retry/lifecycle,
alarms, actual output MMIO/DMA/audio, screenshots, clock changes or physical probes.

All 1341 observations/outliers are retained. Component complete two voices
2.404–2.458 ms, sixteen 4.796–5.388 ms; running service two voices 1.195–2.168 ms,
sixteen 2.463–3.426 ms; boundaries two voices 2.841–23.774 ms, sixteen
4.848–6.108 ms. Counters 709379 Hz, injected logical time/voices. Structural
reduction in duplicate source validation is established. These instrumented
samples do not establish an exact speedup, real-time feasibility or audio
acceptance; a 128-frame 48 kHz span is 2.667 ms and excess/outliers remain.

Next move from these ownership optimizations to native transport integration:
inspect the sampled-clock owner API and existing private E-clock/alarm gates,
then add a bounded nonblocking owner-thread pump with explicit pending alarm/
counter/voice ownership and failure/close retention. Keep strict late/unprimed/
skipped-frame refusal, no catch-up/rebase/retry, and preserve caller-owned contexts
until every native request and both reader barriers confirm. Start with injected
alarm/counter/voice tests; native output MMIO/device/IRQ binding and audible/physical
acceptance remain separate. Preserve unrelated display work; no physical probe.
