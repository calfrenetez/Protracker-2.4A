# Incremental standalone Paula capability preparation

The standalone Paula song owner now begins cancellable analysis and advances one
bounded phase per preparation call. It measures at most 256 timeline ticks,
advances one interval, consumes at most 256 silent frames, or completes and checks
one fixed-size command plan. It retains the full shared 16-track global flow:
selected Paula tracks do not become a separate four-track song.

Initial project and PCM validation, static metadata scans, map validation and
allocation remain synchronous. These happen before playback deadlines. A bounded
operation count does not establish a wall-clock limit or native timing acceptance.

`pt_paula_preflight_begin`, `step`, `transfer` and `close` expose the incremental
interface. The existing `pt_paula_preflight` and `pt_paula_preflight_take` wrappers
remain synchronous and preserve their result and transfer behavior. `PENDING` was
appended to the capability enum, preserving existing numeric values.

Pending and failed reports expose no used-source mask. Only a complete timeline
pass publishes that mask, including checks of late notes. Successful transfer
rewinds and transfers the same audited sequence exactly once, without allocation
or a second measurement. Borrowed project arrays, source storage and allocator
contexts remain immutable and alive through close or transferred sequence close.

The song owner checks its existing snapshot, generation, map and callback guards
before each analysis slice. Failure and close cancel unfinished analysis before
any master promotion. Cancellation releases analysis storage even when adapter
quiescence requires retaining the song handle. After a complete pass, preparation
selectively pins used authoritative masters before readiness; existing bounded
master-copy and playback-cache preparation continue unchanged.

The standalone native editor preparation adapter inherits this incremental gate.
Its device reservation and final release barrier remain separate from capability
analysis. This change supplies no native player loop, timing guarantee, hardware
geometry qualification or listening acceptance.

Tests cover synchronous/incremental parity at 8, 16 and 24 bits, hidden partial
masks, frame/interval bounds, measurement yielding and budget refusal, the same
sequence allocation surviving transfer, repeated-transfer refusal, cancellation,
late geometry refusal, both allocation failures and stale pending song state.
The same cases are included by the native Exec preflight and song fixtures.

Validation at this development step: eight host groups passed with address and
undefined-behavior sanitizers, covering standalone preflight/song/dispatch,
editor lifetime, native editor/transport mocks, and mixed preflight/ownership
regressions. The pinned Amiga compiler built the Exec preflight, song and editor
lifetime fixtures plus prepared-output, transport and Wait wrappers from a
committed source export with only the listed Paula overlays. Those builds are
not emulator, physical-device, native deadline or audible playback acceptance.
The transport and Wait wrappers were cross-built only; their prior runtime
qualification limits remain separate. Runtime/stage regression bounds were kept
unchanged; only pre-playback preparation call budgets accommodate the new slices.

The monolithic native song fixture exceeded the existing 90-second qualification
window, then completed later with RC0 and zero owned Fast bytes. The failed
qualification deadline remains failed; late completion is separate cleanup and
functional evidence. It does not justify increasing the deadline or repeating
that unchanged candidate.

Native song qualification now selects one complete bit-depth matrix using
`PT_TEST_SONG_EXEC_BITS=8`, `16` or `24` with the native fixture entry. Each
candidate retains every case, assertion and runtime bound for its chosen depth,
and prints that depth only after the whole matrix completes. The ordinary host
entry still executes all three matrices. Invalid depths and selection outside
the native entry refuse compilation. Each candidate needs its own exact build
manifest, fresh coordinated window, unchanged deadline and independent cleanup
check. Splitting workloads supplies no native timing or device-output acceptance.

Exact shared030 qualification now passes native preflight (102 Fast allocations,
zero owned bytes), all three full selected-depth song matrices (163 each, zero)
and standalone editor ownership (81, zero). Each candidate completed within its
unchanged90second deadline, with matching depth marker where applicable, RC0,
exact cleanup and subsequent separately locked10second absence/original idle
identity checks. A separately coordinated silent prepared-output fixture also
passes: classic8 zero-leading segment refuses before promotion/output; silent
16/24 use selected zero Chip32 and actual emulator audio.device WRITE, then
confirm device closure and26 Fast allocationszero. Both windows were explicitly
released. See `../evidence/enhanced-editor/paula-incremental/`. These are software
preparation/ownership and silent emulator device checks, not musical timing,
frontend PLAY, physical-device or listening acceptance.

## Optional cancellable initial setup — 5 October 2026

`pt_paula_preflight_setup` owns genuine full-project flow validation and advances
one initial phase per call, charging at most 4096 semantic/metadata items. Fixed
header, span, allocation and cleanup work is additional. The full original project
validates before a pattern-only view is derived; the complete offset mask precedes
event eligibility. No public caller-supplied validation certificate is accepted.

Initial READY is not COMPATIBLE. Transfer publishes an actual audit owner still
PENDING; the complete timeline must pass before its same sequence can transfer.
Private checked reset templates avoid repeated semantic/offset scans at
measurement restart and rewind. Existing synchronous and mutating paths remain
their existing implementations. Successful setup uses three fixed allocation
calls/transient owners, then retains the ordinary audit and sequence owners.

Initial borrowed inputs are immutable during calls/callbacks; between-call edits
change tags. After transfer, project/source remains immutable until all audit and
sequence owners close. Original setup controls can be reused immediately, while
copied allocator callbacks/context must outlive every retained owner. Full declared
capacities, callback alias/reentry refusal and stale cleanup are guarded. Existing
lookahead keeps its sequence-outlives-workspace contract.

Eight focused ASan/UBSan groups pass on the corrected frozen candidate. The first
revision stopped at a compiler warning before an executable was produced; its
original diagnostic and source are retained. Exact assertion-enabled 68000
compiler products are PTRenderStartupTest (88152 bytes) and
PTPaulaPreflightStartupTest (112012 bytes), each built once. Both remain native
NOT RUN. These checks add no editor PLAY, master/cache ownership, backend
activation, native stack/placement, musical deadline, audio or listening acceptance.
See [saved proof and limits](../evidence/enhanced-editor/paula-preflight-startup/README.md).
