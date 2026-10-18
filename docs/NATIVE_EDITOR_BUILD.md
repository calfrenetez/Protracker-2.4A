# Native editor build and stack storage

The editor entry point keeps its view cache, Paula state and nine path buffers
in static storage. Their addresses remain stable for one application lifetime,
and this removes about 15 KiB from main’s automatic storage. The program still
expects one main invocation per fresh executable load.

The change consists of three static qualifiers. Buffer sizes, initial strings,
layout, call order and musical scheduling are unchanged. The previously
uninitialized chosen-path buffer is now zero initialized; it is consumed only
after a successful selection or an explicit recent-path assignment.

Stem export now requests GNU feature declarations only on Linux. This retains
Linux’s existing behavior and avoids conflicting newlib and Amiga timer timeval
declarations in the native build. No SDK header or compiler warning was changed.

The complete clean editor build passes 345 dependency, preprocessing, code,
assembly and inspection stages across 69 C units. The genuine replay object and
runtime libraries also link successfully into a static executable candidate.
The working editor entry point is qualified separately with the existing
editor/view/pattern headers; its display edits are preserved outside this commit.

The clean main’s individual stack report falls from 18,700 to 3,632 bytes;
its frame reservation falls from 18,552 to 3,484 bytes. These measurements do
not establish a whole-task stack minimum. Other routines, nested calls, startup
and system libraries require additional space. A supported startup stack
allocation is being qualified separately.

These results do not qualify application execution, Amiberry GUI behavior,
physical A1200 operation, audio ownership, playback timing or listening.
The compact evidence files in ../evidence/native-editor-build/ keep those
boundaries separate from compiler and linker results.

## Startup failure and screen ownership

The editor's explicit nix20 startup object requests a 64 KiB stack through the
genuine libnix stack module. Its typed anchor retains that module's own startup
and exit constructors. This is an experimental request, not a measured minimum
or a final shipping requirement. The runtime libraries and CRT remain intact.

The selected libnix stack-allocation failure path can report an error before
`main` opens Intuition. When GUI requests are allowed, its original reporter
would then use the editor's still-null manual library base. An app-owned
`__request` provider instead checks
the CRT-owned DOS base, attempts a bounded diagnostic through direct DOS calls,
and returns to the genuine caller. That caller retains its own failure exit.
This is an app-specific link binding, not a general libnix extension API.
Every candidate must prove the selected provider and its callers from its own
link map and emitted code; source-list enrollment alone is insufficient.

Shutdown also checks `CloseScreen`'s result. A refusal leaves the screen and
remaining resources alive, attempts a direct DOS diagnostic and waits in the
original Task. It does not announce a clean exit or return to the CRT. A
successful screen close permits the remaining teardown; the clean-exit marker
is emitted only after that teardown. A retained physical Task requires recovery
appropriate to its still-live resources, not forced executable unloading.

The startup reporter passes nine HOST API-boundary cases and a distinct genuine
native compilation. Screen-close handling passes five HOST cases and separate
clean and working native entry-point compilations. The full working-layout
cohort also passes all 350 stages across 70 C units. HOST stubs establish only
the tested control flow; they do not establish Amiga library or Task behavior.
The focused results are retained in
[startup-request-host.json](../evidence/native-editor-build/startup-request-host.json),
[startup-request-object.json](../evidence/native-editor-build/startup-request-object.json),
[screen-retention-host.json](../evidence/native-editor-build/screen-retention-host.json),
[screen-retention-objects.json](../evidence/native-editor-build/screen-retention-objects.json)
and [working-layout-objects.json](../evidence/native-editor-build/working-layout-objects.json).
The startup data object was compiled with its separately recorded `-O2` profile;
the reporter and application use their recorded `-Os` profiles. The legacy
builder's source-list additions do not prove reproduction of these objects.

The complete working-layout candidate also links and passes its current
provider, calling-convention and HUNK review: all 70 working C objects plus the
separate replay, startup and reporter objects, 116 direct runtime providers,
and 80 ordered link inputs. Its 349,260 bytes and 3,561 relocation records are
fully consumed by the structural parser. The current emitted startup-failure,
screen-retention and replay-call paths are checked separately from the earlier
clean candidate. See [working-layout-link.json](../evidence/native-editor-build/working-layout-link.json).

Exact-candidate Amiberry execution remains a separate gate. Physical A1200,
allocation-failure and stack-restoration runtime checks, audio ownership,
timing and listening remain unqualified until recorded explicitly.

## Checked owner cleanup before a clean return

Normal event-loop completion is now separate from a successful exit code.
Before native teardown, the entry point checks recovery activity, sampler
lifetimes, editor disposal, checked document releases and the local master
allocation ledger. An uncertain owner retains the original Task and its
remaining resources. A closed recovery-file residue returns failure after
ordinary teardown. The clean result requires both successful cleanup and a
zero ledger after releasing the editor descriptor.

The ordinary regression `python3 -B tests/test_native_main_cleanup.py` selects
the actual entry-point decisions and recovery definitions, links genuine
editor/document/sampler/history/workflow code, and passes eleven cases under
ASan/UBSan. Five cases finish; six deliberately retain refused owners until
their separate HOST process ends. Leak-sanitizer was unavailable and disabled.
These are ownership decisions, not simulated native HOLD or reclamation proof.

The clean baseline and working display layout also pass separate genuine native
entry-point compilations, five stages each. Their 64/65 project inputs and
90 selected SDK headers agree between dependencies and preprocessing. Other
objects and the previous full executable are not requalified by these checks.

The updated working executable has since passed a separate static link and
current provider/ABI review. Only main is replaced; 72 other objects retain
their qualified identities. All 116 direct runtime providers and the three new
application references resolve, and the complete 349,152-byte HUNK structure,
relocations, replay and startup/cleanup routes pass saved-byte checks. This is
the working layout candidate; it does not establish a new complete compiler
batch or application execution. Exact Amiberry application execution and
real-A1200 qualification remain outstanding. Native teardown, Task/stack
lifetime, device quiet, audio timing and listening remain separate gates. See
[owner-cleanup-host.json](../evidence/native-editor-build/owner-cleanup-host.json)
[owner-cleanup-objects.json](../evidence/native-editor-build/owner-cleanup-objects.json)
and [owner-cleanup-link.json](../evidence/native-editor-build/owner-cleanup-link.json).

## Isolated environment-helper runtime refusal

An exact helper-only Amiberry run returned RC20 from preparation with empty
output. Its returned marker and snapshot directory were observed; the directory
entry does not inventory its children, so snapshot state creation is unconfirmed.
This was a confirmed refusal, not a timeout or successful setup. Restore and
the editor were not launched. The original failed operation was not replayed.

Separate scoped cleanup stopped only its positively identified emulator and
local service, restored the saved launcher lifecycle files and removed its
exact offline staging copy after emulator absence. A separate observation,
four-fact ownership release and final independent EMPTY check passed. Native
ENV restoration is unconfirmed; that guest domain was disposed with its VM.
See [environment-helper-emulator-refusal.json](../evidence/native-editor-build/environment-helper-emulator-refusal.json).

The updated editor build remains unexecuted. A corrected helper needs its own
HOST/native build and exact-candidate emulator qualification before physical
promotion; the earlier compilation checks do not establish DOS runtime success.

## Corrected environment-helper component qualification

A distinct corrected helper now passes the HOST model, genuine native object
and link checks, and one exact-candidate Amiberry prepare/restore sequence.
The helper changes its newly created directory lock to shared mode through a
checked call before shared-lock operations, and admits DOS V37 or later.
This addresses a source contract defect; the earlier RC20 predicate remains
unconfirmed and its failed operation was not replayed.

Both native phases returned RC0 with the exact transcript and 5,204-byte
snapshot states 1 and 3. Eleven custody objects and snapshot CRCs check out.
All five original environment records were absent in this run and remained
absent after restoration. Present-record cases remain HOST model coverage.
The observed 2 MiB Shell stack is not a measured minimum.

The native sequence completed, but its outer reporter returned failure when
it tried to parse two routine launcher messages as JSON. That failure remains
preserved. A separate saved-data reconciliation verifies the actual native
result, original supervisor closure, 1,813 source facts, 44 extra facts and
unchanged protected inputs. It does not relabel the original outer as passing.

Separate fresh checks then confirmed emulator absence and restoration, released
the original reservation, normally stopped only the original local service,
and independently confirmed EMPTY ownership with both processes absent.
A strict three-line reporter correction is prepared for qualification before
the next application test. The updated editor, physical A1200 and audio gates
remain outstanding. See
[environment-helper-emulator-qualification.json](../evidence/native-editor-build/environment-helper-emulator-qualification.json).
## Updated startup and reporting HOST qualification —18 October2026

The current 349152-byte working editor (`97f9f3a6…`) and 14640-byte
environment helper (`e915664a…`) are now bound into the startup test profile.
One normal-import HOST run passed 37 distinct methods: the existing 19 profile
and input-adapter cases, ten VM-disposal models, and eight new build-binding,
admission and transcript cases. All 123 input facts and protected files were
preserved, all 33 temporary model roots were removed, and supervision closed
with no retained owner. This qualifies models, not an actual editor launch.

A separate HOST run passed 24 transcript groups against each of the two exact
outer parsers. The parsers accept the expected health line, positive owned-
emulator stop line and single final JSON object. Extra, reordered, truncated,
ambiguous and oversized output is refused. The original helper run's outer
JSON error remains recorded as a failure; it was not replayed or relabelled.

Both runs and their evidence boundaries are summarized in
`evidence/native-editor-build/updated-startup-host-qualification.json`.
The subsequent exact Amiberry attempt failed as described below; A1200 promotion
remains pending.

## Updated editor launch refusal —18 October2026

One exact Amiberry attempt prepared the candidate and environment helper, then
received a bridge error at the execution boundary. The launch command was
dispatched, but no launch acknowledgement, editor result, GUI capture or native
environment restoration was confirmed. The service retained a generic error;
the detailed bridge reply was missing from its durable journal. The guest cause
therefore remains unknown. This is not a passing application test.

The failed transaction was not replayed. Separate recovery positively identified
and normally stopped only its emulator and local service, restored the saved
launcher files and removed the exact staging copy. A separate observation
confirmed absence and restoration before releasing ownership. The original
failure and three holds remain in the release archive. All protected inputs and
unrelated display work were preserved. No physical or audio acceptance follows.

See [updated-editor-launch-refusal.json](../evidence/native-editor-build/updated-editor-launch-refusal.json).

## Bridge-error journal regression —18 October2026

The guarded exchange now accepts an optional synchronous error recorder. It
records one validated, unambiguous bridge error before raising the generic
exception. Invalid or duplicate replies cannot become exact error evidence;
recorder failure is terminal and cannot authorize another send.

One HOST run passed 20 tests: eighteen exchange tests, including the original
twelve, plus two private service-wiring tests using the actual journal writer.
The tested helper and eighteen ordinary tests are adopted. The private service
wiring is not installed in a running service. Input, protected-file, guard and
admission checks passed, and supervision closed with no retained owner.

This qualifies error reporting on HOST connection boundaries. It does not
recover the earlier missing reply, establish its guest cause or qualify editor
runtime. See [bridge-error-host-qualification.json](../evidence/native-editor-build/bridge-error-host-qualification.json).

## Fresh editor log-inspection refusal — 18 October 2026

A fresh exact-candidate Amiberry attempt used the updated bridge-error journal.
The bridge acknowledged the launch script, then one `FILEINFO` attempt for
`RAM:<owned-run>/native.log` returned an error. Two preceding directory listings
reported a 64-byte native log, a 50-byte preparation log and a 166-byte control
log, with an empty completion flag. Those sizes do not establish their contents,
editor READY, a frame, native return or restored environment. No capture or key
command was attempted. The detailed error was preserved by both the exchange
and terminal journal; this addresses the earlier diagnostic gap.

An open-writer lock conflict is a possible explanation for the failed file
inspection. The saved reply contains no DOS error number or failed internal
predicate, so that explanation remains unproved. This operation was not replayed.
Separate recovery normally stopped only its identified emulator and service,
restored the saved launcher files, removed exact offline staging and completed
a separately observed four-fact public release. Native environment restoration
remains unconfirmed; the guest domain was disposed with its owned VM. The earlier
launch refusal remains a separate failed transaction. See
[updated-editor-r7-log-refusal.json](../evidence/native-editor-build/updated-editor-r7-log-refusal.json).

The corresponding HOST regression passed 22 methods: eighteen guarded-exchange
cases and four private service error-wiring cases. It qualifies durable error
reporting and terminal propagation at the tested HOST boundaries. It does not
prove the guest cause or editor behavior. See
[bridge-error-host22-qualification.json](../evidence/native-editor-build/bridge-error-host22-qualification.json).

## Private document-switch Task probe progress

The separate Task probe passes 160 native object stages across 32 translation
units over 75 frozen working-source files, including the dirty editor source
and header pair. Dependency and preprocessing sets agree with 48 selected
genuine SDK headers. Its static candidate resolves all 26 direct external
providers; the five real Exec vector wrappers and emitted Task/SP/memory-type
gates were reviewed from saved output. An initial LOAD-path validator failure
was preserved, then reconciled for exactly two doubled-slash spellings before
the two remaining inspection commands ran. No relink was needed.

Eight HOST admission models also pass against the actual admission AST with
local observations. They did not import the service or install its factory.
The probe remains private: no clean-HEAD qualification, canonical probe/frontend
adoption, native runtime, functional allocation-ledger oracle or whole-Task
stack minimum is established. It is not an editor GUI result. See
[document-switch-task-static-host-progress.json](../evidence/native-editor-build/document-switch-task-static-host-progress.json).

## Private live-log sharing probe qualification

A separate private probe now passes eight pure script/policy HOST cases and
eight carrier cases using the actual Plan, Transaction, Observation and
callback with mocked exchange replies. Its single genuine native C unit passes
five object stages, and the 10,948-byte static candidate passes five link and
inspection stages. All eight current providers and CRT/base call paths were
reviewed; the complete three-section HUNK contains 225 relocations. These
results establish HOST control flow and static bytes, not installed-Shell sharing.
See [live-log-share-host-static.json](../evidence/native-editor-build/live-log-share-host-static.json).

The independently reviewed carrier retains the default exclusive-redirection
path and selects append only through its exact private profile. It requires an
early FIRST-only read before terminal controls, then final BOTH markers, RC0
and exact five-file custody. Original guard ownership and VM-disposal ordering
remain in place. See [live-log-share-carrier-source.json](../evidence/native-editor-build/live-log-share-carrier-source.json).

Probe Amiberry runtime, live file sharing and A1200 execution remain NOT_RUN;
this probe establishes no full-editor GUI result. The original R7 failure is
unchanged. Source and test adoption wait for the separate actual target result;
no production feature is enabled by these qualifications. Individual stack
reports and the Delay argument establish neither a whole-Task minimum nor timing.

## Installed-Shell live-log runtime qualification

The exact 10,948-byte LogShare probe completed one Amiberry run on the private
68030 profile. The same Shell appended to a precreated log with `>>`: a read
while the probe was active returned exactly FIRST (22 bytes), followed by the
complete FIRST-plus-SECOND output (45 bytes), immediate RC0 and exact five-file
custody. The independent saved runtime audit is preserved unchanged in
[live-log-share-native-runtime.json](../evidence/native-editor-build/live-log-share-native-runtime.json),
including its historical release-pending state. Earlier descriptive 43-byte
prose was inaccurate; the executable length checks correctly required 45.

The subsequent, separately audited stages observed original VM disposal,
restored launcher files, public ownership release, normal closure of the
identified service and final process, endpoint and staging absence. All 68
protected items remained exact. These later results are recorded separately in
[live-log-share-native-release.json](../evidence/native-editor-build/live-log-share-native-release.json).
The earlier source/HOST/static qualification retains its historical NOT_RUN
boundary; the new results do not rewrite that milestone.

This qualifies the exact component's append/read route on this installed Shell.
The older editor failure cause and full-editor GUI/ENV integration remain
unqualified, as do pre-stop native Task absence, aggregate stack, timing, audio,
physical A1200 operation and listening. No production playback feature is
enabled by this result.


## Private full-editor append profile — HOST qualification

The exact private full-editor append profile and its fixture passed one HOST
run: thirteen pure script cases and two cases using the actual Plan with HOST
models. All fifteen passed. The ten module-origin hashes, 46 input facts, all
68 protected items, guard and admission records stayed exact; original HOST
supervision closed with no retained owner. See
[editor-append-profile-host15.json](../evidence/native-editor-build/editor-append-profile-host15.json).

The suite used the unchanged editor binary as inert data and explicit model
ENVGuard bytes. The actual editor and ENVGuard binaries were unchanged and
were not executed by this qualification. This is source/script/model evidence,
not full-editor GUI, native ENV, live IPC, Amiberry integration or physical
acceptance. It neither establishes the prior FILEINFO failure's cause nor
proves that append resolves it. The profile and tests remain private proposals;
this record enables no production feature.


## Native document-switch Task RAM qualification

The exact 225312-byte standalone document-switch Task candidate passed once
in Amiberry 68030 with a 170-byte oracle, RC0 and an observed 2 MiB stack. Its
32 translation units and 75 source bodies cover New/refusal/cancel/reset,
encoded 24-bit Load/cancel, a real budget refusal, original history/master/save
preservation and a zero final Fast-memory ledger. The five custody files and
100 ordered protocol pairs were verified. See
[document-switch-task-native-runtime.json](../evidence/native-editor-build/document-switch-task-native-runtime.json).

The runtime receipt deliberately records ownership retained at completion. A
separate observer, genuine release, single normal service stop and independent
final check then confirmed empty ownership, original process and endpoint
absence, restored launcher files and all 68 protected items. See
[document-switch-task-native-release.json](../evidence/native-editor-build/document-switch-task-native-release.json).

This qualifies preparation and cancellation in the standalone VM component.
Checked frontend commit/publication remains STOP_UNQUALIFIED; whole-editor GUI,
native device quiet, physical A1200 operation, pre-stop Task absence, aggregate
stack limits, timing and listening are separate acceptance gates.
