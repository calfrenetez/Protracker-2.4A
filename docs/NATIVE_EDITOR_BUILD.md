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
