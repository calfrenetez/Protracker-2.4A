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

## Bounded staging adapter

The pinned AHI interrupt source converts FIFO usage to bytes by multiplying by2;
its hardware header defines2048 playback LONGs. Accessors use16-bit register
reads and32-bit FIFO writes. These are source observations, not Mini bus-cycle
qualification. The Mini register map still needs direct reconciliation before
native MMIO is enabled; no fixed capacity is assumed by the new core.

`amigus_fifo` now stages packed words behind a caller-supplied port. Port capacity
is explicitly normalized to32-bit free words. Poll does at most one capacity
query and one three-word write; less than three free words stalls. Busy submission
preserves the existing block. All writes must have confirmed completion; uncertain
partial writes or capacity faults block further writes until confirmed reset.
A pending/failed reset preserves software state and refuses new audio. Reset also
clears any odd-frame carry, preventing contamination of the next stream.

Its submit/poll/cancel signatures match studio_consumer, but live integration is
not yet wired. Poll completion means copied host data is no longer referenced;
it does NOT mean the device FIFO is empty or silent. After final producer output,
call explicit finish and poll its possible padding, then separately drain/stop
the device. No padding is inserted between blocks. External code must stop the
producer before cancellation and preserve exclusive port ownership.

Host sanitizer tests cover fixed words, stalls, final padding, write uncertainty,
capacity fault, pending/failed reset and successful recovery. Pinned Amiga
compilation passes. No native port, card access, interrupts or performance proof.

## Consumer integration fixture

The real queue/consumer now drives staged packing through a fake FIFO port.
257 stereo frames partitioned1/17/256 produce identical packed bytes plus exactly
one reported final silent frame despite intermittent capacity stalls. A partial
write failure followed by pending/failed reset keeps the queue lease busy until
reset succeeds. All tracked queue allocations are released.

Session orchestration must explicitly reset the FIFO even when the consumer has
no outstanding lease: consumer stop only cancels a transport-held lease. Software
may still retain an odd tail, or hardware may still contain copied audio, after
that lease is gone. The fixture explicitly finishes the tail before normal detach
and explicitly resets after output. Native device drain remains unimplemented.
