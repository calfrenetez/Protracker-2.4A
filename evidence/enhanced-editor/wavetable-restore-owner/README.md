# Injected exact-restore ownership — 27 September 2026

The optional restore callback consumes a complete exact-cursor plan separately
from ordinary start. Restore dispatch requires an idle owner/current bridge
version. Whole-snapshot checks precede acquisition; all active leases and actual
address plans exist before the first callback. Failed preparation releases every
temporary lease, with no voice callback. Loaded unpinned caches or promoted
unchanged master copies may remain. Cache sync may retire stale entries.

A callback owns its cache lease before invocation. Non1 results are uncertain:
unstarted leases are released and all started voices receive bounded stop
attempts. Pending/failed stops retain leases and reservation until confirmed.
Later restores refuse while owner is closing. Exact saved24-bit masters persist.

Host ASan/UBSan: final restore/dispatch fixture PASS6.237s; all three editor
wavetable, legacy guard and Studio fixtures PASS26.128s. Tests exercise16 restored
voices, fractional cursors, missing capability, stale versions, late foreign
source refusal, allocator/upload/cache-budget failures, busy owner refusal,
partial runtime failure and pending/failed/confirmed stop cleanup. No ordinary
start callback substitutes for restore; all16 leases are pinned before first call.

Native build from Git-index source export excludes unrelated display work.
149 dependency hashes match staged sources; native editor main syntax checked.
Binary277764bytes SHA256
`ca1da0f34eb3beadb5884da1f3e4be91f06d82a4f81b5a53902f7ca52693f03a`.
One cross-build exposed a missing-braces warning in the prior snapshot test;
explicit memset initialization fixed it, followed by host regression and rebuild.

Fresh AmiConnect coordination/shared runner run render-files-1790480037972486000
returned0 within90s.156 Fast allocations, zero final owned bytes/no Chip fallback.
All4DMAoff/exact run+launch cleanup and explicit release verified. No remaining
ProTracker hold. No physical/lifecycle/config operation or reset/retry.

The previous renderer snapshot fixture completed late with RC0 and38 released
Fast allocations. Its90s timeout remains failed deadline evidence; guarded cleanup
and explicit release are recorded separately in ../render-snapshot/late-*.

These are injected-driver software lifetime tests, not native card MMIO/output,
real endpoint/stop-fence proof or physical acceptance. Row-range song open remains
refused until silent pre-roll/snapshot restore is integrated into the owned
sequence protocol. Native PLAY does not instantiate this backend.
