# Incremental wavetable preparation — 27 September 2026

Renderer begin/prepare separates static validation from timeline measurement and
refuses next until all measurement succeeds. The wavetable begin/step/close engine
performs one operation per step: at most256 measurement ticks, next/snapshot,
at most256 silent frames, or complete/capability checks. PENDING never authorizes
playback. Terminal reports repeat without advancement. Cancellation releases both
workspaces; steps allocate no memory and pin/upload/start nothing. Synchronous
wrappers retain their public behavior using this same engine.

Coverage: incremental/synchronous report equality including row ranges and late
refusal; copied options; frame/interval progress bounds; long measurement yielding;
invalid-call preservation; cancellation between measurement/traversal phases;
both allocation failures preserving the caller handle; terminal stability. Core
sequence tests prepare at different tick partitions before comparing exact resumed
24-bit audio, including tempo/delay and forward/ping-pong loop phase. Tick-budget
failure poisons the sequence and never permits next. Existing editor and private
EFx ownership regressions passed.

Validation:
- Dispatch ASan/UBSan PASS6.254s.
- Renderer sequence ASan/UBSan PASS3.192s.
- Three index-export editor wavetable/guard/Studio fixtures PASS26.617s.
- Private EFx queued renderer session ASan/UBSan PASS3.466s; pinned mutation
  sequence regression also passed (0.384s).
- Native build and staged native editor syntax PASS;149 dependency hashes matched
  staged sources, generated font separately verified. Binary287172bytes, SHA256
  `2531bad32b8ae88fd9d212b9f71b6c1a3b5ece7b98358b012dc251283d4c87e3`.
- Two concurrent index-export commands briefly collided on Git's index lock.
  The build was retried after the other export completed; no lock was removed.
- Fresh AmiConnect-coordinated shared030 run
  `render-files-1790481270959572000`: RC0 within90s;247 Fast allocations, zero
  owned bytes, budget refusal without Chip fallback. Live target/shared-lock
  guards, all4DMAoff and exact cleanup passed; explicit release sent to AmiConnect.

Limits: initial project/PCM validation, metadata setup/reset and command-time PCM
checks remain synchronous; operation-count bounds are not hard latency guarantees.
Song/editor start still uses the synchronous wrapper. A preparation owner and
responsive native UI integration remain unfinished. Borrowed project/source arrays
must remain immutable until close. No device adapter, real-time scheduling,
audible or physical AmiGUS acceptance. Editor development package unchanged.
