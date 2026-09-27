# Injected-frame whole-song scheduler —27 September2026

The exclusive song/editor driver primes before a future absolute start, with no
voice callbacks during bounded startup commands, silent range pre-roll or restore
cache preparation. At start it requires prior readiness. During playback it
prefetches upcoming plans, consumes bounded elapsed phase, commits at exact ready
deadlines and arms the next positive interval in the same call. All deadlines stay
absolute. Manual advancement cannot bypass the scheduled owner; editor barriers
and close still cancel safely. Clock regression/overflow and late/unready work stop
without issuing a late trigger; unconfirmed voices retain device/master pins.

Host ASan/UBSan instrumented wavetable7.461s PASS with the final independent
renderer oracle for EVERY boundary plus total output duration, whole/range/lead-in
starts, fractional131->137BPM tempo, zero-frame startup, no early/late callbacks,
readiness repeats, missed preparation, regression, overflow, bypass refusal,
cancellation and uncertain stop. No PCM/project validation calls during driver
service. Staged editor/legacy guard/Studio28.788s PASS before final test-only oracle
addition; scheduled Stop/dispose cancels unstarted exact-restore leases.

Native build/main syntax PASS,164staged inputs/generated font verified. Binary
367628bytes SHAc4bbc87b95cb2be6604ba810a3e92e8ab5d622746dcee07827ea7c9d2858e54a.
Shared030 run render-files-1790503808820693000 PASSED within90s RC0;1141Fast
allocations, zeroowned/noChipfallback. Final native fixture includes independent
boundary oracle. All4PaulaDMAoff, exact cleanup true and independently absent.
Explicit release sent to AmiConnect. No lifecycle/physical operation.

This is an injected-clock software driver, not native timer/event-loop integration.
Caller must poll before and AT returned boundaries; synchronous callbacks and clock
sampling do not guarantee physical execution time. Native clock conversion/wakeup,
callback-duration accounting, actual device binding and sustainable audio timing
remain unqualified. Native PLAY/card output stays disabled. Initial overflow check
conservatively reserves all preflight frames, including silent pre-roll. Unexpected
post-start zero/silent spans refuse rather than inventing timing. Master precision,
persistence and accepted classic layout are unchanged.
