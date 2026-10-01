# Mixed playback accessor cost diagnostic

1 October 2026. Production code is unchanged from `3bbc8ae`. The native fixture
adds four read-only repetitions of five groups after committing the first mixed
batch: both genuine current master pins (retain/unpin), Paula prepared-current,
AmiGUS backend-current, all held Paula lease locations, and all held AmiGUS lease
locations with serial/data, valid/version and backend address checks. Each repeat
asserts unchanged live-voice bytes, total cache pins, output count, allocation
count and bus write count, then revalidates the complete mixed owner.

These 120 observations are **not** 120 additional playback scenarios. The native
fixture still runs 15 cases: six 2/16-voice component cases at 8/16/24 bits, six
complete two-interval running cases, and three cancel/reuse cases. The full host
suite passes 198 scenarios under ASan/UBSan in 8.167 seconds. The native-only
observation code is excluded from that host suite. An initial cross-build caught
a missing private prototype include; it was fixed before creating this exact
qualified source tree. No failed candidate was launched.

Indexed tree `7e248400ef941adf2e6f040217314f855b8a3eb5`: 162 source hashes verified
against the candidate and tree, zero generated sources, three verified binaries.
`PTExecMixedOwnerTest`: 240332 bytes, SHA-256
`c4598e380b2cb176964ac7172d5319eabdd100dd039ceb4cbc5cc3219ba62e57`.

One shared030 run `1790836778909873000` passed in 16.312 seconds overall, within
90-second fixture, 140-second runner and 200-second orchestration bounds, including
cleanup, under the coordinated 240-second reservation. Normal done/return-code 0,
171 Fast allocations with zero owned bytes, Chip release, all reader/master/cache
ownership and temporary workspace release, private unsubmitted E-clock/device/
request closure passed. Exact run/launcher cleanup and an independent subsequent
locked process/profile/bridge/68030/running/all-four-DMA-off/path-absence check
passed. Explicit release was acknowledged by AmiConnect and forwarded to peers.
No lifecycle, retry, alarms, real output MMIO/DMA/audio or physical operations.

All 1341 observations, including all outliers, are retained in `native.log` and
`observations.json`. Accessor groups have 12 observations each per voice count:

| Group | 2 voices (ms) | 16 voices (ms) |
| --- | ---: | ---: |
| Both master retain/unpin checks | 0.186–0.196 | 0.186–0.196 |
| Paula prepared-current | 0.118–0.121 | 0.120–0.123 |
| AmiGUS backend-current | 0.054–0.055 | 0.054–0.055 |
| All Paula live locations (1 / 4 leases) | 0.154–0.156 | 0.482–0.488 |
| All AmiGUS live locations (1 / 12 leases) | 0.120–5.594 | 0.867–5.729 |

Counters use 709379 Hz. Measurements include assertion and counter sampling
costs and injected ownership callbacks. They isolate **active accessor groups**,
not the whole prepared-ready function, candidate-ready checks or real hardware
bus costs. Outliers cannot be treated as normal deterministic execution cost.
They establish no actual-time or audible playback acceptance.

The next implementation candidate is sharing Paula bridge metadata validation
within one serialized ready-check call, retaining every per-lease serial/valid/
version/address/alignment/range check. `current()` already validates the bridge;
`prepared_location()` currently repeats it for every active and candidate lease.
Any private validated-location helper must require immutable arrays/contexts,
perform no callbacks, and never carry validation across public calls. Standalone
location APIs must keep full validation. Stale generation/descriptor/lease tests
and complete-song ownership tests remain mandatory before adoption. Native PLAY,
event-loop, output device/IRQ binding and hardware/audio acceptance remain open.
