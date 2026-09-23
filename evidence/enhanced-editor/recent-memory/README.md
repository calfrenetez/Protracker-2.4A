# Recent-file memory checkpoint

Three targeted host tests PASS, including allocated and legacy APIs, all record
truncations/single-bit corruptions, allocation refusal, short I/O/EINTR and a hard
write failure preserving the previous generation. Pinned native build PASS.

Guest import fixture failed before recent fixture was run: AmigaDOS refused the
host-style same-file mutation attempt while a read descriptor remained open.
The assertion aborted the launcher, leaving no done marker; runner timed out.
No import/recent emulator pass claimed. C:Status confirmed both fixture/launcher
had exited; exact owned cleanup completed under fresh shared guards, window
explicitly released to AmiConnect. No UI, reset, retry or physical operation.

Next separate host-only mutation checks from native fixture before requesting a
fresh bounded window. Recent-memory fixture remains pending guest validation.
Unrelated prepared-display work remains unstaged and unqualified.
