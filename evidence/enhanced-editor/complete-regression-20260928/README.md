# Complete committed-source regression, 28 September 2026

Immutable source export87c383339fcb8a2ac967f24204abd21f12232fb7 passed all201 host
tests in475.074s (exit0). Git-dependent fixture exports used a separate temporary
index and explicit export worktree, preserving the original index and unrelated
display edits. The complete native build produced139executables. All288 listed
source hashes match the commit; all139binary sizes andSHA256 hashes independently
match the manifest. Build success does not assert all139programs were executed.

This snapshot includes the callback-status, interrupt-lifetime and Studio storage
quiescence changes. Their exact native emulator runs are retained in the adjacent
amigus-callback-status, amigus-interrupt-owner and editor-studio-quiescence evidence.
The later1ed99ac drain-status fix is separate: its13 host checks and seven-case
native execution appear in amigus-drain-status. Do not attribute it to this older
full-build snapshot. No physical tests, actual IRQ/card/MMIO or native Studio
output qualification are implied. The physical Amiga remained off and unprobed.
