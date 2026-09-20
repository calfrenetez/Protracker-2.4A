# dev39 — sample-offset range evidence

A separate 140-byte replay diagnostic records stored sample pointers/word lengths,
loop ranges, offset memory and the range at each actual trigger write. Pointers
are normalized relative to the serialized MOD; owned silence uses FFFFFFFF.
Existing pitch/control records remain the first52 bytes. Register and MOVE.L
flags, including X, are preserved by the two trigger-write wrappers.

Eight fixtures confirm the pinned replay applies 9xx twice on a note row but
loads the initial DMA range between those applications. No-note 9xx changes the
stored range once, and delayed passes do not repeat it. 900 reuses memory;
instrument reload restores ranges but retains offset memory. An offset at or
beyond remaining length sets length to one word without advancing start. Loop
ranges are unaffected, and nonzero loop starts initially limit length to loop end.

## Validation

- 49 host checks passed: 48 regressions in 80.724 seconds plus the new range-model
  comparison. All previous binaries are byte-identical to dev38.
- Final run `offset1789929725382188000`: 17 native captures in 46.576 seconds.
- Eight fixtures, 152 ticks per pass, repeated byte-for-byte. The independent host
  model matches every stored/trigger range, loop, offset and trigger count on all
  four channels. Boundary, reload, inheritance, delay and independence cases pass.
- Baseline first52 fields match dev38 exactly; no old evidence is rewritten.
- The preliminary run completed captures but was rejected by a completion-file
  visibility race. The driver now waits for exact marker contents. The wrapper's
  counter increment was also changed to address arithmetic to preserve X; final
  evidence is from the rebuilt diagnostic only. Both runs received guarded cleanup.
- Final release at 18:43:26 UTC verified no emulator process/HDF holder/socket and
  exact launcher restoration. AmiConnect received explicit release.
- PTSampleTraceTest: 61208 bytes, SHA256
  `79fac7cf4bf97ac61966a1fd0d2af960efa76ea4e50d58a1191cab4bf1eb79ee`.

## Acceptance boundary and next work

This is diagnostic evidence, not newly enabled sample-offset playback. Shipping
9xx remains refused. Next implement persistent range state and initial segment
handoff to loops, then compare reference PCM with these trigger/loop snapshots.
Enhanced sample precision/slice policy requires explicit definition. The accepted
screen and every shipping binary remain unchanged. No physical hardware or audio
acceptance is claimed; no physical hardware was accessed.
