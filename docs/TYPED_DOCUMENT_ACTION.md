# Opt-in typed document actions

The focused fixture accepts `case <name>` to run one of its thirteen normally
cleaned-up action cases without the mixed/checked prerequisite suites. Names are
listed in `ACTION_CASES` in the Python driver. It flushes a case-specific BEGIN
line before entering the case and a PASS line only after assertions and cleanup
return. Invalid or incomplete diagnostic arguments return 20 with no markers.
The separate `editor_document_action_case_test.c` executable always selects
`admit-candidate-overlap`, for launchers that take no fixture arguments. The
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
both typed-action and typed-replacement options enabled; native execution and
frontend/device acceptance remain pending for this candidate.
