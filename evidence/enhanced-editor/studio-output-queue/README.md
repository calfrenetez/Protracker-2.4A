# Bounded true24 output queue

Single allocation,1..8 blocks, <=256 stereo24/48k frames per block. Copies retain
master precision, full queues refuse, and one consumer lease protects storage
until matching-ticket release. Finish drains; abort discards only unleased audio;
close refuses while leased. No concurrency/interrupt/DMA/device-transfer claim.

Host fixture covers exact24-bit values, source independence,32 ring cycles, full
pressure, stale/wrong tickets, busy close, finish/drain and abort retention. Studio
regressions run alongside it. PTStudioQueueTest is included in native cross-build.

No emulator/physical test in this milestone. Native build includes preserved
unrelated display work; not editor UI/release qualification. Queue is not yet
connected to the song producer or a device consumer.
