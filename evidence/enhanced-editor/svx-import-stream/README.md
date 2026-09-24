# Bounded IFF sample import

Host ASan/UBSan IFF codec and streamed import suites pass. Eight combinations
cover compressed/uncompressed, named/fallback and forward-loop/no-loop inputs;
exact multiblock PCM agrees with the independent original decoder, including
Fibonacci wrap. Name, rounded volume, loop endpoints and trailing PCM retained.
Three allocator refusals, fill/limit/multioctave rejection, interrupted/short
reads, exact undo/redo and zero leaks pass. RAW/WAV and sampler regressions pass.
Full pinned native build passes.

Shared030 run1790234259150663000 rc0/PASS after fresh AmiConnect clearance and
shared lock/live guards.80 Fast allocations, zero owned bytes, budget refusal
without Chip fallback. Sample staging clean, exact owned run cleanup and explicit
release confirmed to AmiConnect and waiting AmiWBMonitor task. No physical,
card, audio, lifecycle or configuration operations. No UI/physical acceptance.
