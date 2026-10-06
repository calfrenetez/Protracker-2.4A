# Portable preparation-session compiler qualification — 6 October 2026

The exact new `PTEditorPrepareSessionTest` compiles and links for m68000/softfloat
with the pinned Amiga nix20 compiler/runtime, assertions enabled. Product is
395508 bytes, SHA256
`b9b9a71351a012f20b5dc67228604f2f6e02d7dce93d07b2fed28fcb8cf83d10`.
It is **NEVER_EXECUTED** in Amiberry or on physical hardware at this milestone.
Its expected692-byte stdout is copied from the successful host fixture; it is
not native runtime output.

The complete assertion body and81 support units were built from915 frozen source
files. All91 compiler calls passed with empty stderr, each bounded to120 seconds,
with a600-second overall bound and15.648-second actual build. Five tools, seven
runtime inputs and233 compiler-reported dependencies are pinned. Independent
saved-byte verification covers1105 retained files, current four source/test
controls and all16 protected work paths. No target or shared infrastructure action
occurred. Root separately verified retained bytes and all tool/runtime/dependencies.

The first portable attempt rejected misleading loop indentation in the test;
it produced no product and remains FIRST_FAILURE_RETAINED. The current distinct
attempt uses a separately reviewed formatting-only correction and a fresh complete
six-call ASan/UBSan host PASS. Both compiler manifests and the original diagnostic
are preserved here. No first result was rewritten or target test retried.

This is compiler evidence only. Exact native runtime, physical CPU/software,
allocation placement, stack/step timing, device capacity/order/completion/stop,
IRQ, musical scheduling, audio/listening and full native UI/PLAY remain separate.
The earlier physical editor-bridge fixture is a different frozen candidate.

Run `python3 -B verify_saved.py` for saved byte/relationship verification only.
The executable and private source snapshots are excluded. The archived builder
is an operational custody record referring to its original private directory;
do not replay it from this packet. Later execution needs a fresh reviewed window,
peer coordination, current ownership and live guards.
