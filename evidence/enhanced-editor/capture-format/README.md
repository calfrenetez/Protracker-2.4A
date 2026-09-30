# Exact capture-format gate

Four isolated host tests passed (14.415 seconds); six capture fixtures plus the guard built for Amiga. All 126 indexed source hashes and seven binary hashes matched the build manifest.

The 30 September coordinated emulator attempt was refused at Guest construction with `Not the shared 030 emulator`, before guest commands, staging or launcher creation. No fixture executed; the remaining two cases were skipped. No guest resources were acquired, no retry or lifecycle operation was attempted, and the window was explicitly released to AmiConnect. The earlier approval-limit attempt never launched.

These checks cover exact declared format validation before allocation or PCM reservation, and retained stop/ownership behavior. Actual device format negotiation and physical capture remain unimplemented/unqualified. Native execution of this change remains pending.
