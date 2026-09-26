# EFx one-shot extension — 26 September 2026 UTC

Pinned reference: vendor/pt23f/replayer/PT2.3F_replay_cia.s.
Three fixtures contain a nonzero master first word, classic one-word repeat,
EFf followed by EF0, and ordinary, E92 or ED3 subsequent rows. Two native
captures each stop normally after 24 ticks. Portable flow/private PCM comparisons
match all 18 active ticks per capture, retaining source bytes unchanged. Trace
one-word snapshots contain two bytes and 14 zero pad bytes; long-loop layout is
unchanged. Pointer comparisons remove one fixed allocation relocation only.

Reference run: invert-shared-1790463683132379000, result.json.
Core run: render-files-1790463786239302000, native-result.json.
Native core independently checks every output frame for mixed forward-loop and
one-shot channels, inherited new notes, E91, ED1, EF0, an unselected/muted EFx
channel, nonzero immutable masters, all allocation failures and insufficient
private budget. Both runs completed with RC0, all four audio DMA channels off,
exact owned cleanup and explicit coordination release. No lifecycle changes.

build.json records the focused canonical m68000/nix20 build, compiler safety
workaround and source/runtime/binary hashes. It is not a full clean editor build;
unrelated local display work was neither staged nor qualified by these tests.
The 12 renderer and two bounce host regressions passed with sanitizers. Separate
ordering tests compare both these captures and the three existing loop captures.

No new native editor or export CLI package is qualified by this core run.
No physical Paula audio/performance or AmiGUS acceptance is claimed.
