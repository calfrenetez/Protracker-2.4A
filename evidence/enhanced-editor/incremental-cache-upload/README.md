# Incremental cache-upload jobs — 27 September 2026

PCM and AmiGUS adapters now provide begin/step/cancel. Begin validates and allocates
without packing/bus writes. New leases remain unpublished/pinned; each step writes
at most256bytes or supplied staging, whole-frame rounded. Only full completion
publishes/transfers the lease. Hits transfer immediately without writes. Failure
or cancel releases partial resources without changing result leases or masters.

Source descriptors remain checked; values/data are caller-owned immutable through
finish/cancel. The core jobs do not acquire sampler pins. Existing synchronous
wrappers use the same jobs; prepared song retains its exact master pin throughout
those calls. Song/editor yielding and pin ownership across turns remain follow-on.
AmiGUS jobs additionally guard reservation identity and live ownership before steps
and before/after writes, including the final write; no partial playback address.

Validation:
- ASan/UBSan upload suite0.357s PASS: public/private and incremental conversion
  parity across8/16/24-bit masters,8/16-bit outputs, channel/endian/padding and
  variable capacities; cancellation at every partial byte, failure at every write,
  descriptor/invalidation/alias/capacity refusal and pinned-old-version retention.
  A600-byte conversion with1024-byte staging takes exactly3 steps/writes.
- Adapter suite0.633s PASS: every partial cancellation, odd-byte device-word
  assembly, hit ownership transfer, detach busy, swapped reservation, lost ownership
  before/final write, partial transfer cleanup. Public/private legacy cases pass.
- Public PCM conversion0.331s; instrumented wavetable6.773s PASS (no project/PCM
  validator calls in prepared playback). Existing invalid-public-input checks remain.
- Staged editor wavetable/legacy guards/Studio suite27.658s PASS.
- Native build/editor-main syntax PASS;159 staged inputs/font verified.
  Binary330588bytes SHA256
  8e3c920e5a64eb5111429cc07845bccca6406b8da38a9fede7e8d64ae448d95e.
- Coordinated shared030/DevBench run render-files-1790486902936333000 RC0
  within90s. New upload-job and editor ownership fixtures PASS;548 Fast allocations,
  zero owned bytes/no Chip fallback. All4DMAoff and exact cleanup verified;
  explicitly released to AmiConnect, no recovery hold or physical operations.

Byte limits do not bound allocator/public-validation/driver-callback wall time.
No native PLAY, scheduling, card transport, audible or physical acceptance.
Unrelated display work and packaged editor preserved.
