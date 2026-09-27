# Four EFx clocks during pattern delay

Two classic mono 8-bit fixtures exercise all four mutation clocks, shared bank
collisions, instrument changes and repeated tick zero during EEx pattern delay.
Only channel 0 plays a note; channels 1–3 contribute mutations without audible
voices. Both samples have 18-byte loops, exercising cursor wrap.

- `inv_delay_four`: EE2 repeats the middle row twice. EFf remains active on all
  four clocks. Delayed tick zero records channel masks 1,1,2,2,4,4,8: seven writes
  in reference execution order. There are 30 active ticks / 28,800 stereo frames.
- `inv_delay_disable`: EE1 repeats the middle row once; EF0 disables channel 2
  until the next row re-enables it. Delayed tick zero records 1,1,2,2,8, with no
  disabled-channel write. There are 24 active ticks / 23,040 stereo frames.

The existing 208-byte diagnostic is unchanged. Each actual reference store is
logged into its fixed eight-entry buffer; overflow remains an explicit failure.
The pinned source calls mt_CheckEffects once per channel on non-fresh ticks.
That path calls mt_UpdateFunk once, then EFx at counter zero can call it once
more. Each call has one byte store and no loop. Thus four channels permit at
most eight stores per tick through these paths; the EEx fixture observes seven
because one channel's row command is the pattern delay. This is a source bound
for the pinned replayer, not a hardware timing claim.

Reference run `invert-shared-1790471382699652000`: four RC0/native-stop captures
within the 90-second reservation. Both normalized traces match their repeat;
no overflow. All DMA off and exact cleanup confirmed, then explicitly released.
Diagnostic SHA256: d2fa17c56b1ef2531a042c9d29bd10e1e784d398548a797e2add00606f4871e8.
Raw logs, normalized traces, fixtures and build identity are retained here.

Host sanitizer regression: 23 tests PASS in 53.402 seconds. Every private-bank
byte matches after each active tick. Independent logged-store PCM matches
offline and pull1/17/256; all contributing clocks survive exclusion, muting and
solo. Actual WAV, individual/grouped stems, bounce/cancellation, undo/redo and
master save/reload agree and preserve every original master.

Native runs use PTExecInvertWriteTest, built with 45 dependency identities in
native/build.json. The --delay runner requires one named --case and a fresh
180-second reservation; it cannot accidentally launch both cases as one batch.
Run host checks with `python3 -m unittest discover -s tests -p 'test_invert*.py'`.
Capture with `tools/shared_infra_invert.py --delay`; run each native case with
`tools/shared_infra_invert_handoff.py --delay --case <fixture-name>` only after
coordination and live shared-harness guards. Native pulls are17/256; host adds1.

No production source change was needed. This qualifies existing private EFx
behavior; it does not add16/24-bit EFx admission, enable native Studio/device
output, or establish physical Paula sound, A1200 performance or AmiGUS behavior.

Native software evidence:
- inv_delay_four: invert-handoff-1790471541627009000, RC0 within180 seconds.
- inv_delay_disable: invert-handoff-1790471701096076000, RC0 within180 seconds.
Both offline/pull17/256 and mute/solo comparisons pass, each with44 Fast
allocations, zero owned bytes and budget refusal without Chip fallback. Each
window confirmed all four DMA off and exact owned cleanup/absence, then was
explicitly released to AmiConnect and Scott. No timeout, retry, reset or hold.
Native SHA256: c675c1fc51bca7b08bc9f85e8a18cdfba4635bdea744f732ffc3d6cb369a9480,
91,180 bytes. Production dependency hashes match committed application sources.
