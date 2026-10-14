# Opt-in typed document actions

The focused fixture accepts `case <name>` to run one of its thirteen normally
cleaned-up action cases without the mixed/checked prerequisite suites. Names are
listed in `ACTION_CASES` in the Python driver. It flushes a case-specific BEGIN
line before entering the case and a PASS line only after assertions and cleanup
return. Invalid or incomplete diagnostic arguments return 20 with no markers.
The separate `editor_document_action_case_test.c` executable defaults to
`admit-candidate-overlap`, for launchers that take no fixture arguments. Build
it with `PT_DOCUMENT_ACTION_CASE` defined as a quoted case name to select one
other case. The host recipe also checks separate `publish-new` and
`publish-load` executables, each requiring its exact two markers. Keep these
as distinct candidate binaries and qualify each one independently. The
original focused fixture's no-argument invocation still runs the complete prerequisite/action suite;
an individual case never prints the full-suite completion line. Alias and
uncertain-state modes still deliberately retain their quarantined allocations
and are separate host subprocess tests.

The `actions` selector and separate `editor_document_actions_test.c` executable
run all thirteen normal-cleanup action cases with individual flushed markers
and a distinct component completion line. They exclude prerequisite suites and
the deliberately retained alias/uncertainty cases. Component completion must
not be reported as a pass of the original combined suite.

These selectors prepare bounded native diagnostics; host success does not
qualify native execution. The earlier combined New/Load native invocation
returned no confirmed completion. Its approved recovery restored the launcher
and released ownership, but did not establish a fixture pass or its failure
cause. Retain that evidence separately from later diagnostic results.

Enable `PT_DOCUMENT_TYPED_ACTION=1` together with `PT_DOCUMENT_TYPED_REPLACEMENT=1` and include `editor_document_action.c`. This private serialized HOST composition adds detached NEW and byte-load actions. Ordinary editor NEW/load/PLAY behavior is unchanged.

Provide a zero action, an empty detached candidate, the genuine current checked owner/binding, zero future establishment/owner/bridge controls, allocator callbacks, and full living spans for named opaque contexts. An empty candidate may retain its allocator after a previous checked release. The candidate header must be disjoint from the complete action, old storage/controls, input bytes and every named context. The action may lie wholly inside a named parent span, but its allocator context pointer must lie outside the action. Admission rejects overlap before publishing the action or initializing the candidate.

`pt_editor_document_action_new` and `pt_editor_document_action_load` prepare the complete candidate before any STOP. Negative core vetoes protect full-capacity old document storage, intrinsic sampler lifetime storage, genuine old owner children/parents, both engines/caches, future controls, the action, its private comparison record, named contexts and byte input before staging writes. The exact private comparison record and captured old headers detect observed control changes. No caller Boolean, copied owner or terminal result grants release authority. Allocator extent/alignment and callback non-destruction remain contracts; this guard cannot undo arbitrary callback writes/frees or infer unnamed opaque context bounds.

`PREPARED` keeps the old owner, header and storage intact. Cancel before the first step ends the still-PREPARED typed replacement and invokes checked candidate release once. Confirmed cancellation returns `CANCELED`. A normal preparation failure cleans its known staging entries and returns `FAILED`; the old document remains intact. Both terminal controls have `self == NULL` and retain `result` plus the precise core `preparation_result`. These observations convey no ownership or closure authority. Reset the ended genuine action with `pt_editor_document_action_reset` before preparing another action. Reset never frees allocations and refuses an adopted or uncertain action. Permanent spatial exclusions protect the private comparison/control storage even when its self bytes are zero. Reset also refuses overlapping active controls and the adopted candidate.

The first step invokes the existing typed replacement. A STOP attempt is irreversible even when it returns `WAITING`; cancel then refuses. The existing adapter owns actual closure, sampler retirement, checked old release, one publication, and owner construction at the final document address. `COMMITTED_NO_OWNER` preserves a published document after confirmed quiet when new-owner construction fails. Finish still requires genuine new owner/cache closure, editor disposal and final sampler retirement through the existing typed finish guard.

Observed reentry, changed old state or an uncertain core staging/release result retain `HOLD`. The genuine action, documents, original controls, contexts, byte source and outstanding obligations remain alive. No retry, forced cleanup, rollback or quiet claim follows. Known source/candidate aliases that core can safely classify as borrowed may instead return ordinary `FAILED` without releasing the borrowed address. Core staging quarantine makes candidate checked cleanup refuse before allocator callbacks; a non-OK core result does not convert retained obligations into cleanup authority.

Run the portable public recipe:

```sh
python3 -m unittest discover -s tests -p test_editor_document_action.py
```

The private HOST fixture compiles 84 units with assertions, ASan and UBSan. It covers thirteen prewrite-admission/reset, NEW/load failure and preservation, cancel, WAITING and final-address publication groups; nineteen allocation-alias cases, including expanded sampler table metadata and an unrepresentable return after one accepted child; three isolated reentry/stale-state HOLD cases; and two private zero-self reset exclusions. It retains the original fifteen mixed regressions, thirty-nine checked lifecycle cases, eighteen checked allocator-alias cases and ten typed replacement groups. HOLD subprocesses deliberately keep reachable obligations for process lifetime. This evidence does not qualify native ABI, device output, memory placement, scheduling, physical hardware or listening.


Native build preparation on 14 October found a 238,220-byte compiler-reported
local stack requirement in the admission helper. Admission now builds only its
span inventory locally and captures the full editor/owner snapshots directly
into the already-validated output. Every prewrite check completes first; no
allocator callback, allocation or additional global workspace is introduced.
The selected m68000/soft-float/libnix build reports 2,236 bytes for that helper
after this change. These individual compiler records do not establish a complete
call-chain stack bound or a minimum safe task Stack.

The public HOST recipe also compiles the action translation unit with
`-Wframe-larger-than=4096` under its existing warnings-as-errors and sanitizer
flags, so an accidental large automatic snapshot fails the check. All functional
and negative lifecycle groups still pass. The 84-unit native fixture links with
both typed-action and typed-replacement options enabled. The original combined
fixture still has no confirmed native completion; frontend/device acceptance
remains pending.

On 14 October, the separate `publish-new` candidate passed on Amiberry 68030
with both exact markers, RC0 and the queried 524288-byte Stack. It exercised
successful New publication at the final document address, owner construction
and cleanup using injected voices and ordinary RAM allocations. Exact staging
cleanup, launcher restoration, process/transport absence and ownership release
were independently verified. See the scoped
[native evidence](../evidence/enhanced-editor/document-publication-native/result.json).
This qualifies that single lifecycle case; it does not activate the frontend
or qualify physical memory placement, device output, scheduling or listening.

The separately built original `publish-load` candidate passed its host checks but its
native invocation reported `STILL RUNNING` at the unchanged 90-second limit.
It has no confirmed native RC or completion markers. Its subsequent scoped
recovery restored the saved launcher files, removed the exact staging copy,
independently verified process/endpoint absence and released ownership. This
observation alone does not establish a hang, assertion failure or eventual
completion. The earlier thirteen-case native component also remains
unconfirmed; its recovery did not convert it to a pass.

Incremental action steps now retain the original comparison snapshot and update
only the expected phase, busy and result fields. The complete before/after
metadata comparisons remain; the replacement subobject continues to use its own
typed guard. Recopying unchanged editor/history/owner snapshots at both edges
of every step added substantial memory traffic. An instrumented host fixture
observed 502 steps for Load and 73 for New. The selected m68000 layout is
236,504 bytes, so removing those two copies eliminates 237,450,016 bytes of
snapshot copying over 502 steps. This calculation is not a native timing
measurement or proof of the earlier timeout's cause.

Three additional isolated sanitizer cases corrupt a captured editor field or
input field between delegated steps, or corrupt the snapshot from an old-storage
release callback. All must retain HOLD without granting cleanup. These supplement
the existing preparation reentry, source-staleness and ownership regressions.

The candidate containing only the snapshot-copy reduction also reported
`STILL RUNNING` at 90 seconds. Its native result remains unconfirmed; its separate
approved recovery restored the launcher and independently released ownership. The time limit has not been widened.

The separately inspected runtime archive contains an unoptimized `memcmp`, but
the original action translation unit already emits its own optimized local
comparison. The archive alone therefore does not explain this failure. A private
byte-equality experiment passed host checks but was withdrawn without a native
run or product change because a performance benefit was not established.
The separate `cancel-load` diagnostic prepares and cancels before STOP,
excluding final publication and incremental owner construction.

On 15 October, the exact fixed `cancel-load` candidate passed on Amiberry 68030
with both expected markers, RC0 and a queried 524288-byte Stack. Exact staging
cleanup, launcher restoration, process/transport absence and ownership release
were independently verified. Preparation/cancellation is qualified for that
fixture; positive Load publication and its later owner construction still lack
confirmed completion. Physical hardware and full frontend acceptance remain open.

Metadata equality now checks even-addressed snapshots in four-byte chunks,
with exact byte comparisons for odd addresses and the tail. Every byte and
padding byte remains checked; the replacement subobject exclusion and all
ownership checks are unchanged. Loads use `memcpy` for aliasing safety. The
68000 alignment hint follows `project_snapshot.h` and is used only after a
runtime even-address check. The target compiler emits longword loads/comparisons.

The complete host sanitizer recipe passes with comparison tests for all paired
input alignments 0–3, exact allocation lengths 0–73 and 65535/65536/65537/236504,
and mismatches at small-buffer positions and large-buffer boundaries. The
new comparison candidate is cross-built; native completion is not inferred
from these host or assembly checks.

The changed longword-comparison candidate reached a guarded native SCRIPT call,
but the harness returned an error and its saved exception chain includes lock
contention. No native return code or completion markers were accepted. This
run has no `STILL RUNNING` classification, so it is not evidence of another
90-second timeout. Dispatch and completion remain uncertain. Interrupted
ownership is retained pending separately approved cleanup and release.

A separate `publish-no-owner` diagnostic is cross-built to check successful
Load publication followed by a rejected output format during owner creation.
Its expected `COMMITTED_NO_OWNER` result keeps the published document usable
without granting playback ownership. The existing host case passes; this
exact diagnostic has not run natively.
