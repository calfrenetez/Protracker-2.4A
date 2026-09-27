# Exact cache-cursor restore plans — 27 September 2026

pt_amigus_render_restore translates an active ordinary mono one-shot/forward
software voice to original aligned playback bounds plus an absolute cache-byte
Q32 cursor. It retains every source phase bit for8/16-bit caches without changing
24-bit masters. Fractional/odd cursors are separate from aligned bounds; these
are software descriptors, not verified register writes. Ordinary start cannot
consume the restore plan by dropping its cursor. Future drivers must explicitly
support exact restoration or refuse before output.

Validation rejects inactive/malformed phase and cycle state, segments/handoffs,
pingpong/stereo, invalid source/cache formats, overflow and unrepresentable bounds.
All refusal paths preserve the output object. There is no allocation, PCM read,
callback, device access or pin; cache/source ownership remains the caller's duty.

pt_wavetable_restore_preflight checks explicit restore capability and every
active snapshot source identity/geometry before a caller can upload/start. Its
sample mask identifies only active slots; foreign descriptors and late-channel
failures refuse. This gate is standalone. Current song open still rejects ranges;
no playback callback or native device path consumes these plans yet.

Host ASan/UBSan exact-restore fixture PASS0.340s. Frame-by-frame source-position
oracles cover head/loop/one-shot boundaries and8/16-bit cache conversion;32-bit
fraction recovery is exact, including the last fraction near address-space bounds.
Whole-snapshot gate and existing wavetable/song/ownership regressions PASS6.040s.
An initial test incorrectly expected a forward-loop exclusive end below the RAM
limit to fail; corrected to test a genuinely overflowing cache allocation.

Native restore fixture cross-build PASS:32492bytes,SHA256
`b48bdd868775d782df65396d3b1a88e03cdf38baa4617468191ae015c9908244`.
33 dependency hashes cover the built fixture plus syntax-only dispatcher check;
compiler/runtime digests recorded. No native run was attempted. Prior renderer
run render-files-1790479010316536000 still lacks completion/rc/log and remains on
a coordinated recovery hold; no retry/reset/cleanup or new guest operation here.
No emulator runtime, real card output, audible or physical acceptance claimed.
