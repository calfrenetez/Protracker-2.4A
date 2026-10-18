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
The new full editor's exact Amiberry GUI run and A1200 promotion remain pending.
