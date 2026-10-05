# Checked native memory prerequisite

`src/native/native_checked_memory.c/.h` provides an optional task-side checked
allocator for future ordinary-RAM native diagnostics. Its integrated host model
passes with the current clarified header. The current real-NDK object build also passes. Earlier original-header proofs
remain separately scoped historical evidence.
The helper is not wired to editor PLAY. A separate native entry exists only as an
unimported source draft under review, with no launch eligibility. Authoritative
8/16/24-bit masters, selective derived Paula/AmiGUS caches, exact scheduling and
the classic baseline retain their existing contracts.

## Bounded ownership and guarded extents

A genuine zero-initialized control records at most **32 live allocations** and
copies at most **16 caller guard spans**. New returned intervals must be disjoint
from the entire control, every known guarded capacity and every live allocation.
The original guard descriptor vector need live only through initialization.
Copied descriptor identities and extents are immutable; their nonempty pointee
extents remain alive and stable through finish or the retained process lifetime.
Controlled contents may change through their own serialized owner APIs. Sample
bytes keep their independent immutable-source contract. This distinction permits
guarded task/clock/alarm/control storage to perform its legitimate operations; it
does not grant permission to change guard identities, move storage or edit live
sample bytes.

The caller must independently own the control itself and keep it alive. A guard
must not encompass or overlap that control; initialization refuses such an alias.
Separate full source, control, backend, allocator and output capacities may be
guarded. Unlisted opaque extents remain the caller's disjointness responsibility;
the finite guard list is not a global alias detector.

Initialization uses the existing master-memory policy but refuses its Chip-only
choice. It requires `MEMF_FAST`, preserves the policy's 256 KiB Fast reserve and
clamps the reserve-aware ceiling to the caller's budget. Zero budget is valid.
Initialization allocates nothing and preserves caller control on refusal. The
shared policy's existing Chip-only behavior remains unchanged.

Only `PREPARE` permits allocation. Zero/overflow requests, full registry,
accounting faults and insufficient budget or available memory refuse before
`AllocMem`. A successful request uses `MEMF_FAST | MEMF_PUBLIC`, charges exactly
the payload bytes and checks base/final-byte `TypeOfMem` results. The endpoint
checks and native allocation contract are not a per-byte placement scan. No size
header is stored in or read before the payload.

A non-NULL return with uncertain representation, type, aliasing or callback
outcome is neither published nor automatically freed. Its possible interval is
retained and the control enters irreversible `RETAIN_ALL`. It does not become a
new ownership certificate. Native object compilation does not prove that actual
Exec memory placement or allocation/release behavior has been observed.

## Release and lifecycle

`pt_private_native_release_checked` accepts a NULL no-op on a valid active
control, or an exact live base pointer. Foreign, interior and duplicate pointers
are not probed for a header and are not passed to `FreeMem`. Only the registry's
exact pointer/size is freed after accounting and placement checks; underflow
refuses. The void allocator callback latches the same failure as the integer API.

Seal ends allocation permanently; an allocation attempt after seal latches
retention. Sealing alone is not release permission. After a source is exposed,
the caller must independently prove exact source shutdown, command detachment
and reader retirement before releasing their respective storage. The allocator
creates no source-quiet, adoption or retirement receipt.

Mutating operation reentry while busy latches a sticky fault and disables future
allocation/free. A completed `FreeMem` consumes its known entry even if reentry
then latches retention, so that freed pointer is not retained or freed again.
Readonly stats refuses while busy; that refusal is not a mutating reentry latch.
All operations require serialized original normal-task entry. Initialization OS
queries are covered by that obligation; recursive pre-initialization access is
not detected. No allocator/release/owner operation runs from IRQ.

`finish` frees nothing. Success requires exact zero live ownership and charged
bytes, no ambiguous return and no sticky fault. `OWNED` leaves the phase intact
so independently permitted exact releases can finish later. `RETAIN_ALL` has no
recovery, unseal, bulk-free or automatic cleanup path. Refusal never permits
destroying a live control. Stats publication guards the entire control, guarded
and live intervals, and any representable ambiguous interval.

Any uncertain exposed source requires retaining the whole original Process,
HUNK code, libraries, stack, callback/context and possible sample lifetimes. The
entry needs a separately reviewed non-killing hold and recovery protocol. This
helper never aborts, exits, waits, changes vectors or invokes a hold itself. Heap
retention alone cannot protect unloaded code.

## Eight-path integration and recorded evidence

The lean repository layout adds exactly these eight code/test paths:

- `src/native/native_checked_memory.c` and `.h`;
- `tests/native_checked_memory_test.c`;
- `tests/checked_memory_host_stubs.c` and `.h`;
- `tests/checked_memory_host_sdk/exec/memory.h`;
- `tests/checked_memory_host_sdk/proto/exec.h`;
- `tests/test_native_checked_memory.py`.

It uses the existing unchanged `src/native/master_memory.h`; no duplicate policy
or second production implementation is installed. The two test C includes and
Python source/include paths were narrowly adapted for this layout. The current
header revision changes comments only: API/types/macros/noncomment tokens and
line count are unchanged. The recipe's first docstring uses timeless wording.
Allocator, stub and fixture behavior, three ordered translation units and
sanitizer flags remain unchanged.

| Scope | Recorded result |
| --- | --- |
| Current clarified-header integrated host V2 | **PASS once:** one group, one test invoking 14 unconditional ownership/policy cases under ASan/UBSan. Compile and runtime RC0; three TUs and three full dependency queries; nine stable candidate files, eight canonical and 69 SDK dependencies; eight exact original/durable pairs. Root and independent saved-byte reviews are CLEAR. |
| Original 17-path host model | Historical PASS once with the original header and its original paths. It is preserved separately; it is not substituted for the fresh integrated result. |
| Original-header real-NDK ordinary object V1 | Historical PASS once: 18 tool commands, one full dependency query, one 68000/software-float C object, 39 exact saved pairs and 15 local stack annotations. No link, executable or native execution. |
| Current clarified-header real-NDK ordinary object V2 | **PASS once:** 18 tool commands, one full dependency query, five stable inputs, three canonical and 39 real-NDK dependencies; one ordinary object, 39 exact saved pairs and 15 local stack rows. No link or native execution. This is a separate current-header build. |
| Native entry/application wiring | Separate unimported source draft under review; **NOT QUALIFIED**. Editor PLAY unchanged. |
| Native execution, Amiberry and real A1200 | **NOT RUN for this helper.** Real Exec placement, cleanup and source quiet have not been observed. |
| Complete task/IRQ stack, residency and WCET | **UNKNOWN; 65,536-byte launch NOT CLEARED.** |

The current integrated host model is 166,224 bytes, SHA256
`b65a6942982594f92ba0733dd16ae14b323c437b417036648c174b6d55227e08`.
Its 14 cases cover the implemented budget/reserve/payload accounting, no-Fast and
zero-budget refusal, malformed/wrapping extents, 32-entry pressure, known returned
aliases, wrong endpoint class, poisoned-prefix header avoidance, unknown/interior/
duplicate release, accounting/reentry faults, sealing/retention, guarded stats
outputs and finish. These are finite software-model cases, not exhaustive
mutation or native OS coverage.

The historical object is 3,268 bytes, SHA256
`c573bea6a9b112c2a99d65a3463bf9d06d2194ec38ddf545b998390c02b94b4d`.
The current-header object is also 3,268 bytes, SHA256
`ae775092acc44e992716ccdbbc71f9a6ba86aaece899cef12e1db71bc58d399f`.
Only the embedded ordinary-object output path changes one metadata byte; saved
instructions and relocations are identical. Both builds report 15 local `.su`
rows, with a largest 512-byte initialization frame, dynamic and bounded. This
is a per-function fact, not an aggregate call-chain, library-vector, task-stack
or IRQ/system-stack bound. A larger task stack cannot qualify the separate
interrupt stack. The object format contains `HUNK_UNIT` metadata; no executable
HUNK was linked or run. An initial saved-audit refusal from assuming the old
whole-file hash is preserved; no compiler retry occurred.

Saved evidence is in
[evidence/enhanced-editor/native-checked-memory](../evidence/enhanced-editor/native-checked-memory/README.md).
Its verifier inspects saved bytes without extracting or running products. Tool,
runtime and host-SDK hashes are uncopied provenance; the archive does not
independently qualify those external bytes. Genuine entry bootstrap, complete process hold,
actual source ownership/shutdown, native memory placement, IRQ ABI/exclusion,
stack/residency/WCET, guarded emulator/physical runs, DMA/device behavior, output
timing, audio and listening remain separate requirements. Existing architecture
is not declared complete by these tests.
