# Odd-frame Paula playback copies

Host: five focused sanitizer groups pass from the isolated indexed source tree.
The initial host invocation raced tree extraction and found no test modules;
that failed discovery log is retained separately. No tests ran in that attempt.
The rerun after extraction completed passed all five groups.

Native initial attempt `1790563601961108000` FAILED with normal PTPaulaTest RC20
at the new first-row assertion; four other fixtures RC0. The test sampled after
only five VBlanks while the replay initializes its counter to0 and waits speed6
before fetching the first row. The revised test observes ticks/period for at most
20 VBlanks and logs state; production code is unchanged. This explanation does
not turn the earlier failed attempt into a pass. DMAoff and exact run/launcher
absence independently verified before explicit release. No reset/retry occurred.

Fresh native run `1790563750046698000` PASS: all5fixtures RC0 in69.25s under90s.
The first note was observed at7ticks in allthree new precision cases; five-frame
masters produce six-byte Chip caches, zero tails, safe loop/sync invalidation and
exact saved masters. CIAA fallback NOT RUN because Workbench owns timerB; existing
vectors preserved. Owned allocations/timers/audio released, allDMAoff, private
paths removed, subsequent independent locked check and explicit RELEASE passed.
Native132004 SHA256`a38f43d93d01391708b91bb2657044f4e0e2ea61c81dba91041db93738457231`.
Editor265184 SHA256`badc183fbecf3111a8f08fc9801a02e923dca7f74dd4e08929b3bf1cc51c9969`.
Editor functional-only run `1790563905483835000` PASS with3tracks and255-frame
16/24-bit masters: correct DMA mask, low-bit edit-stop, exactundo, patternrestart,
exactsave/reopen/play/second save. Bothnormal exits RC0; allfiveENV exact
presence/byte restoration, allDMAoff, private path cleanup, independent subsequent
check and explicit RELEASE verified. Captures/visual NOT TESTED in this scope.
Prior screenshot failures remain unresolved; editor scope is functional
only, with no capture or visual acceptance. Physical Amiga is OFF and untouched.
