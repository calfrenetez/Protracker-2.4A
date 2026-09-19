# AmiGUS public interface snapshot

Only the public C structure/constants and SFD call declarations are included.
They are unmodified from `necronomfive/AmiGUS-pub` commit
`d8c9a0429f41cd5f3dbadae34ef438e45c9c3718`; they also match release `rc-6`.
See `../../amigus-sdk.lock.json` for hashes and upstream location.

The header identifies LGPL version 3 only. The driver project's original
`COPYING` and `COPYING.LESSER` are retained here. Its readme explicitly identifies
LGPL 3, whereas the tools elsewhere in that repository use GPL 3. No utility or
driver implementation source has been copied into this project. AmiGUSTest uses
the installed, replaceable `amigus.library` through its public vectors.

`src/diagnostic/amigus_calls.h` supplies the three GCC call macros needed now.
Their offsets/registers follow the SFD: FindCard -30/a0, ReserveCard -36/a0,d0,d1,
FreeCard -42/a0,d0,d1. Interrupt entry points are deliberately not bound yet.
