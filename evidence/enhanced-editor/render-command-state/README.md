# Shared effect-command state refactor

All112 host tests PASS in192.581s; pinned native build PASS. No emulator/physical
run this milestone. The focused renderer test also passed before the full suite.
Direct comparison confirmed effect/range-trigger/gain/tremolo bodies unchanged
apart from shared type names. Reference streaming uses the shared state/functions.

This is a low-level prevalidated command-state seam, not a finished command-intent
plan or Studio adapter. It still borrows immutable PCM and may partially advance
state on failure. Studio ownership and abort rules must be honored by future
translation. No native editor/live transport/performance qualification.
