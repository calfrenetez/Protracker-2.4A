# Physical disabled FIFO/count/nonempty reset — PASS

Separate physical run1790894672726472000, exact AmiGUSTest0.6
6c87f24a1d9e3d4395c39a884d4c203980383bf7641be3c9ec3e6ae9dc8bd5f5,
23760bytes, qualified first through shared030 and committed b88516f.

On Mini firmware7ea663e7/hardware0: initial flags/mask/format/rate/usage all0;
three zero-data long stores while playback disabled reported pending words2/4/6.
One-shot disable/reset confirmed empty on the first read-only poll. Ownrelease
confirmed1/retained0, AHI restored resident1/supported1/delayed0/users0/ahi0.
Known driverID and installedCRC unchanged, exact RAM cleanup plus independent
absence PASS. DevBench returned to connected localhost original running68030
with all four Paula DMA channels off; explicit release sent, no retained hold.

This verifies disabled long-store word accounting and nonempty reset for six
zero words only. It does not verify capacity, physical sample data ordering,
playback, interrupt teardown, scheduling, acoustic silence, recording or listening.
No installed file/configuration/firmware changes, playback enable or IRQ install.
Native initial filesystem failure and guarded recovery remain in their own tier.
