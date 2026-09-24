# Bounded enhanced-project reader core

Host ASan/UBSan PASS: exact re-encode of mixed v1 fixture (24-bit stereo,
loops, slices, MIDI endpoints, optional extensions); bounded callbacks;
original parser parity for truncated files and byte mutations with original
and repaired CRCs; every preflight/decode read failure leaves output descriptor
unchanged. Decoder writes unpublished staging only. Existing project suite PASS.
Pinned native fixture cross-build PASS. No emulator/physical or UI proof here.

Core API only: document transaction and native/CLI file-load integration remain
unfinished. Existing enhanced-project loading still retains encoded input.
Source must remain stable; no concurrent-write snapshot guarantee.
