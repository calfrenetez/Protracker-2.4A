# Import/recent native behavior validation

Corrected import fixture preserves concurrent shrink/growth checks on the host;
AmigaDOS prevents the required simultaneous write reopen, so native runs explicitly
report those cases as host-only. Production import code is unchanged.

Four targeted host tests PASS (import plus three recent tests), pinned native
build PASS. Individual shared030 import and recent-memory fixtures return0 with
PASS/done markers; exact owned files cleaned and AmiConnect window released.
Previous assertion/timeout evidence remains in ../recent-memory/.

These fixtures use their tracking heap allocator, not the Exec-backed wrapper.
They prove descriptor/file behavior, refusal and cleanup; they do not establish
Fast/Chip pointer placement, runtime library allocation policy, native UI release
quality or physical A1200/AmiGUS performance. No physical or UI operations.
