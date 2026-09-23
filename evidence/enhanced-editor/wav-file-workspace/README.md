# Allocated WAV file buffers

Native WAV and per-stem WAV file state/path/header/encoding/comparison blocks now
use the caller's bounded allocator alongside the mixer. Legacy entry remains
stack-based. Host ASan/UBSan file/stem suites passed (2 tests,17.350s); exhaustive
allocation failures passed (1 test,2.668s), now4 WAV and10 two-stem allocations.
Two live blocks (file and render) are checked for balanced release on all paths.

Pinned native build passed. Separate shared030 allocated-WAV and memory-failure
runs returned0/PASS within each90-second deadline, with exact hashes in results.
Final done markers preceded exact owned cleanup and explicit AmiConnect release.
No UI/startup/restart/physical operation. Header-only contract comments were
clarified during the build; executable source stayed fixed during validation.

Compiler stack.su: allocated WAV entry76 bytes, legacy6244; file helper144,
encoding sink68, byte comparison48. Per-function estimates, not call-stack peaks
or physical performance. Stem batch-level buffers/runtime internals remain open.
Latest full105 suite remains802ebc7; affected suites were run for this change.
Unrelated dirty display work preserved; no release/editor UI/live Studio/card
acceptance follows from these file tests.
