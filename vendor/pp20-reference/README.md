# PP20 algorithm reference

This is the unmodified `src/depackers/ppdepack.c` from libxmp at the commit in
provenance.json. Its file header explicitly places the code in the Public Domain
and credits Stuart Caie, Heikki Orsila and Claudio Matsuoka; all notices are kept.
It is retained for provenance and is not compiled into ProTracker.

The adaptation in src/core/pp20.c keeps those credits, replaces pointer-before-start
reads with bounded indices, checks length extension before addition, validates
matches against bytes already produced, and preflights the complete bitstream
without output before decoding. PX20 encryption is not implemented.

The pinned 2.3F assembler uses powerpacker.library version 35+ via ppLoadData.
That dependency is not bundled or called by this enhanced decoder, and its legacy
assembler path is unchanged. No GPL or decompiled original compression code is
included. The downloaded upstream musical regression fixture remains local only;
its identity and expected decoded checksum are recorded for repeatable validation.
