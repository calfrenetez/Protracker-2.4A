# Phase-only command-state advancement

The bounded voice advance API reuses the audio reader's phase transition without
PCM reads, audio mixing, allocation or callbacks. Studio dispatch comparison now
uses it after successful output blocks to keep its borrowed command mirror in sync.

Six targeted host tests passed: voice traversal/mix/partition, phase-only advance,
segments, Studio mixer, Studio command dispatch and Studio interval reader. Tests
cover fractional/large steps, loop seams, one-shot ends and pending sample handoffs.
PTVoiceAdvanceTest is included in the native build. No emulator or physical run
was performed. This is not a CPU/deadline, live-output or editor release claim.

Full song scheduling and source-lifetime ownership remain caller responsibilities;
the mirror must advance by exactly the successful Studio output frame count.
