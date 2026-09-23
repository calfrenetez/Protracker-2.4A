# Bounded render verification reads

Host ASan/UBSan render-file and stem-file tests passed (2 tests, 11.750s).
Additional render run forces repeated EINTR and reads of at most7 bytes.
Checks cover exact true24 PCM, short-write failure, cancellation, corruption,
truncated/trailing staging, no-replace races and cleanup. Pinned native build
passed. Shared030 PTRenderFileTest and PTStemFileTest returned0; result.json
records the exact candidate hashes. Guarded run files cleaned, AmiConnect
explicitly released. No UI, startup, restart or physical operations.

No full-suite rerun or release-package claim. The native editor built in the
same invocation includes unrelated dirty display work and is not qualified by
these file-test binaries. No live Studio/AmiGUS or physical acceptance claim.
Descriptor reads remove the stdio verification buffer; runtime internal
allocation and native stack placement are not fully budget-qualified.
