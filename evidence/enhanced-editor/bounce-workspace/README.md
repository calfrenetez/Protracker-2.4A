# Bounded sample-bounce workspace

Host test_bounce.py passed under ASan/UBSan (1 test,4.280s). It injects all five
fresh-bounce allocation failures and all four with an existing table/redo chain,
including workspace refusal after output PCM staging. Checks preserve master
samples, report, sample table, history cursor/revision/count and live allocation
count. Preserved redo then succeeds. Exact true24 output, PTG identity, mixed undo,
cancellation, budget refusal and eviction checks remain covered.

Pinned native build passed. PTBounceTest SHA256 is recorded in result.json;
shared030 execution returned0 with BOUNCE PASS within the90-second bound. Final
synchronous done marker preceded exact owned-run cleanup and explicit AmiConnect
release. No UI/startup/restart/physical operation. The runner's generic historical
scope label mentions render/stem; this invocation ran only --bounce-only.

Latest full105 host suite remains the preceding render-workspace milestone; this
change ran the affected bounce regression. Native editor includes unrelated dirty
display work and is not a qualified UI/release candidate. No live Studio or actual
AmiGUS acceptance. Header contract wording was clarified after native execution;
no executable code changed after validation.
