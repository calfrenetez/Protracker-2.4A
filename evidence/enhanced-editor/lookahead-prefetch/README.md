# Immutable lookahead and cache prefetch — 27 September 2026

A Fast workspace previews upcoming renderer commands on copied phase/state in
<=256-frame steps, then resolves one plan. It never consumes the live interval.
Live consume may interleave; commit requires the same fully consumed interval.
Monotonic interval IDs reject old jobs after ordinary completion/next/rewind.
Cancellation changes no live state; mutable/private EFx sources refuse preview.

The master-pinned song/editor prefetch path prepares the next command batch's
selective cache leases while the current interval elapses. Ready prefetch invokes
no voice callback. Early completion refuses; ready completion checks identities
then transfers state/leases without conversion/cache allocation. Stop/edit/dispose
cancels preview/upload work before active-voice stop confirmation handling.

Validation:
- Host ASan/UBSan renderer sequence3.350s PASS: full audio/command parity across
  forward/pingpong loops, tempo/delay, whole/range/pre-roll modes and1/17/256-frame
  partitions. Predicted plans match ordinary completion; preview-committed playback
  matches reference audio. No PCM value-validator calls during preview/commit.
- Instrumented wavetable7.160s PASS. Light native-reused preview fixture covers
  exact plans/phase, live-consume interleaving, early/double commit refusal, repeated
  ready output, zero/oversized steps, stale jobs/rewind, cancel after owner disposal,
  and private mutable-source refusal.
- Song fixtures prefetch an upcoming600-byte source while prior voices remain
  active; verify no early callbacks, no writes at ready commit, refusal of stale
  completed source, partial-write failure and cancellation with an old voice's
  stop unconfirmed. Existing range restoration/lease ownership tests pass.
- Staged editor/legacy guard/Studio28.097s PASS, including Stop/dispose through
  prefetch and edit/undo through ordinary pending uploads.
- Native build/main syntax PASS;162 staged inputs/font verified. Corrected binary351728bytes
  SHA256 48c25760db003ec252cd6ee316e1c7aec8cb28f31fb4d816be714c936cac0a44.

Lookahead has borrowed immutable sources; song pins provide ownership. Caller must
supply real elapsed time and enforce deadlines. Initial allocation, bounded plan
construction and device callbacks remain synchronous. Native clock/event loop,
late-service policy, card transport and physical performance are unfinished.
The heavy render-sequence native fixture was not rerun; full audio oracle evidence
above is host-only. Emulator evidence uses the smaller editor/ownership fixture.
No classic layout/package change and no physical AmiGUS acceptance.


## Failed emulator run and retained recovery hold

The ORIGINAL candidate d3e76a239c9cb896485747aaa74db538cff6fba8a273f1df242e369f1b00328e
ran as render-files-1790488754500248000 and exceeded90s. Preserve result.json as
passed=false/run_files_cleaned=false. Read-only recovery found individual fixture
PASS markers followed by native_memory_finish asserting native_pool.used !=0 and
Program aborted, without done/test.rc. All4DMAoff was observed but is not proof of
clean exit or allocator integrity. Task inspection returned mostly unknown names;
window inspection returned no windows. Exit remains unverified.

The new test fixture used calloc, bypassing native malloc remapping, but its free
was remapped to the native pool releaser. This allocator mismatch can pass foreign
storage to FreeMem and invalidate pool accounting/guest integrity. Both new test
allocations now use malloc+explicit zeroing; host renderer/staged editor tests and
native rebuild passed after correction. Corrected bytes HAVE NOT RUN IN EMULATOR.
Original/failed and corrected build manifests are retained separately.

No automatic retry, reset, requester dismissal or guest cleanup. Exact staging
and recovery hold retained with AmiConnect; shared guest must not be reused merely
because the test later exits. A coordinated clean restart requires a new user
recovery decision. No physical operations, no claim of product leak or emulator
acceptance for this candidate. Host-only software changes may be reviewed/pushed.
