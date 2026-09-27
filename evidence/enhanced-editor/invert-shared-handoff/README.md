# Shared EFx mutation across instrument handoffs

Three fixtures, two captures each, run on the coordinated shared030 guest:
`invert-shared-1790469444343872000`. All six executions returned0/native-stop,
completed within90 seconds, matched after fixed allocation relocation, and were
cleaned up only after all four DMA channels were off. The reservation was released.

The heard channel starts sample1 with EF8, changes to sample2 without resetting
its EFx speed, then returns to sample1 with EFf. Channel1 starts on sample1 with
EFf but never plays a note. On row1 it either joins the heard channel on sample2
or stays on sample1; on row2 it selects sample2. One variant uses a nonzero-master
one-shot for sample2. Thus both clocks can address one bank or independently
mutate two banks while the voice completes its current iteration.

The separate188-byte diagnostic preserves the earlier164-byte fields, appending:
-164: channel1 cursor (32-bit relocation-relative pointer)
-168: speed/gliss byte;169: accumulator;170-171: padding
-172-187: channel1 loop bytes (16), or its one-shot word plus zero padding

The original diagnostic source is byte-identical. The shipping replay is not
modified. `diagnostic-build.json` pins the generated diagnostic, dependencies,
assembler, compiler and runtime; `tools/build_shared_invert_trace.py` rebuilds it.
The normalizer derives stable sample bases from channel0 (which visits both),
then maps all recorded addresses by those same bases, including channel1's
cursor. Raw logs retain original addresses. Corruption checks ensure the added
clock/bytes remain visible; aliases must show identical snapshots.

Host checks in `host-tests.log`:11 tests PASS in27.861s, including earlier
handoff/command regressions. Both clocks and both loop snapshots match on every
active tick. A separate PCM oracle applies all recorded mutations before mixing:
all17280 stereo24 frames match offline and pulls1/17/256. The unselected mutator
still contributes; muted and solo-excluded variants also match. Actual WAV,
individual/grouped stems, bounce/cancel, undo/redo and master save/reload preserve
all source samples and generated24-bit output.

Production sources are unchanged. This is qualification of the existing private
classic-mono8 behavior, not new16/24-bit EFx admission, native Studio output,
physical audio, analog fidelity or a realtime performance guarantee.

Native qualification: `invert-handoff-1790469661898922000` completed all three
cases RC0 within the reserved420 seconds. Offline/pull1/17/256 and mute/solo
variants matched. Each reports49 Fast allocations, no Chip fallback and zero
owned bytes at completion. All four DMA channels were off before exact cleanup;
owned run/launch absence was rechecked and the reservation explicitly released.
No timeout extension, retry, UI, profile, lifecycle or physical work occurred.
Native binary SHA256:
`adba923a25cf1fd70cf3313cc4a8bc3d9838bc0f18fc25e77f5a44a736a986df` (89984 bytes).
The45 dependencies are pinned in `native/build.json`; every production dependency
matches the committed application. The only build_core_tests change is the new
native shared-clock test target, isolated from unrelated display work.
