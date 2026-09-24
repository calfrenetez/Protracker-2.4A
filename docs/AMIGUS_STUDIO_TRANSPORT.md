# Studio PCM transport review and packing

24 September 2026. Review targets the existing pinned public source commit
`d8c9a0429f41cd5f3dbadae34ef438e45c9c3718`, not an unverified latest version.
The local SDK checkout matches that commit. No card, firmware or driver was touched.

## Supported path found

The public library SFD exposes discovery, reservation and interrupts, not a
high-level streaming submission API. The pinned AHI driver implements separate
24-bit PCM playback; this is independent of the 8/16-bit wavetable engine.

Primary pinned references:
- [Mode table](https://github.com/necronomfive/AmiGUS-pub/blob/d8c9a0429f41cd5f3dbadae34ef438e45c9c3718/Software/Drivers/AHI4/src/amigus_ahi_modes.c): stereo24, six bytes per frame.
- [Register definitions](https://github.com/necronomfive/AmiGUS-pub/blob/d8c9a0429f41cd5f3dbadae34ef438e45c9c3718/Software/Drivers/AHI4/header/amigus_hardware.h): format5, FIFO write offset0x0c, endian/channel flags.
- [Packing function](https://github.com/necronomfive/AmiGUS-pub/blob/d8c9a0429f41cd5f3dbadae34ef438e45c9c3718/Software/Drivers/AHI4/src/copies.c): PlaybackCopy32to24 emits three32-bit words for four samples. Its AHI inputs are left-aligned32; our signed24 masters are right-aligned and must not be truncated again.
- [Worker](https://github.com/necronomfive/AmiGUS-pub/blob/d8c9a0429f41cd5f3dbadae34ef438e45c9c3718/Software/Drivers/AHI4/src/worker.c): underrun recovery uses full playback initialization to restore24-bit stereo alignment.

## Implemented now

`amigus_pcm_pack` independently packs numeric FIFO words from48k stereo24 PCM.
It preserves all24 bits, interleaves L/R, and emits MSB-first words. No native
register access or upstream implementation code is included. Two stereo frames
form three words. A trailing odd frame is retained across blocks; explicit finish
adds at most one silent frame and reports it. No hidden padding between blocks.
Capacity/format refusal preserves state and outputs. Each call handles at most256
frames with caller storage, no allocation and no master edits.

Tests independently decode output bytes against the source at1/17/256-frame
partitions, check fixed extreme-value words, final padding, repeat finish,
capacity refusal and format rejection. Host ASan/UBSan and pinned Amiga compilation
pass. This packer is not connected to the consumer or an actual card yet.

## Remaining before live output

Verify Mini-specific register access widths, FIFO usage units/capacity and safe
interrupt/cancellation sequence against Mini documentation and driver accessors.
Implement bounded FIFO capacity handling and explicit alignment/reset state before
wiring submission. Submission completion must distinguish copied host memory from
still-audible device FIFO contents. Reserve PCM exclusively and unwind ownership
on every failure; do not equate host lease release with device silence.

No emulator positive-card, physical Mini, interrupt ABI, underrun recovery,
bus throughput or68030 real-time deadline is qualified. A24-bit DAC alone would
not prove24-bit source playback; the pinned PCM implementation is the basis here.
