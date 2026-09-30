# Mixed service validation and native cost

1 October 2026. Full borrowed project/source/master/identity validation runs at
EACH public service entry, once per serialized call. File-local validated helpers
share only that call's successful check; no state or permission survives return.
Standalone private staging entries retain their own full checks. Active/prepared
voice snapshots, genuine pins, cache versions/leases and all addresses remain
checked before output; strict deadlines, bounded staging and partial-failure
retention/stop behavior remain unchanged. Source edits/reentry inside callbacks
are forbidden by the existing contract.

ASan/UBSan host189 PASS7.922s (174 previous plus15 cases60..64 at8/16/24): stale
metadata before clock binding, changed master tokens and driver identity between
ready/start calls, active metadata/master changes retain readers after uncertain
stop, no further clock read/output and no repeated stop. Separate mixed preflight
PASS6.468s. Selected isolated build162source hashes/zero generated/3binaries
verified against indexed tree b6a1f26baf495c2cb809c80d89cfdeb327664adc.
Native PTExecMixedOwnerTest228696bytes SHA256
d45651c1e2afd616bd1dcaa1a4cd13834960308abace5f5cc6d9f04ec67b0aaa.

Native ONLY21 cases:6 cost modes58..59 plus15 stale modes60..64 at8/16/24.
Real private unsubmittedUNIT_ECLOCK observations around injected logical time and
fake voice/bus callbacks; no alarms, actual output/MMIO/DMA/audio or physical test.
Startup2voice4.689..4.725ms versus prior6.898..7.250ms;16voice7.873..8.172ms versus
prior10.006..10.389ms. Ready observations mostly0.94..0.96ms, first1.28ms and a
24-bit2voice21.30ms outlier before the clock sample. All outliers retained in raw
observations; this is neither reliable cadence nor exact/native timing acceptance.
No deadline policy relaxation or catch-up/rebase.

ONE first actual guest run1790809725715209000 PASS15.606s overall;231actualFast
allocations/zero owned bytes, nativeChip release assertions; all readers/pins/cache
leases/private counter/device/request closed. Fresh AmiConnect reservation and
inactive peer snapshots, standard shared locked live guards, exact owned cleanup,
independent later locked running68030/bridge/all4DMAoff/run+launcherabsence PASS,
explicit RELEASE. Bounds200s overall/140runner/90fixture/reservation<=240s.

Earlier host sandbox refusal occurred before staging or guest launch (ps blocked
inside constructor); retain separately as NOT RUN. Process/lock access resolved,
original window ended and fresh separate coordination preceded first guest launch.
No automatic guest retry/reset/lifecycle or physical operations. No binaries tracked.
