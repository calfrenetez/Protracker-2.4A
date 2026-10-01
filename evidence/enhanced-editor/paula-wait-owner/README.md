# Native notification-driven owner — host/build, 1 October 2026

`src/native/paula_pump.h` owns a started transport on the exact calling Task.
Binding verifies all three private timer ports belong to that Task and claims
one transport owner. Duplicate/copy/wrong-task bind, step and close refuse;
transport restart/reattach refuses until explicit pump close succeeds. No task
priority change or frontend installation.

Each step checks a mandatory nonoverlapping owner termination mask without
clearing signals, performs ONE actual bounded transport service, and returns
WORK while forecast/debt remains. Only successful readiness permits Exec Wait
on private boundary/periodic notifications plus termination. WAKE requires
another actual service, never signal-derived or synthetic logical time. Existing
actual read/grid/256-frame entry/post watchdogs and exact boundary rules remain.
No Wait0, catchup/rebase, global mask clearing or unconditional unfinished WaitIO.
The caller bounds work iterations and supplies abort delivery; timer notification
and cooperative termination cannot guarantee recovery from a stuck foreign call.

Abort/DONE/error attempts stop once. HOLD retains pump, transport, editor/master
storage and all callback contexts; further step remains HOLD without service,
Wait or automatic retry. Explicit close performs bounded progress until reader,
cache/device/timer closure is confirmed, then clears the pump claim. It does not
detach/dispose borrowed editor storage. Task identity changed after Wait retains
HOLD; only the original task can finish cleanup. Failed closure vetoes edits.

Host sanitizer transport + native-editor suites PASS24.323s after final ownership
guards. Cases include wrong/current/changed task, duplicate owner/copy/restart,
invalid termination-mask collisions, fresh actual service after wake, abort before
service and during Wait, no wait while forecast work remains, late wake refusal,
pending timer abort and active DMA retention, explicit close progress and complete
song traversal through modeled exact boundaries. Mock Wait advances actual test
counter to earliest owned timer target; this is no native timing/output proof.
Initial assertions that close must finish in one call were corrected: real owner
semantics legitimately retain a pending shutdown. Boundary arm adds its existing
fourth clock read; expectations now distinguish three/four actual reads.

Pinned cross-build PASS;154 compiled dependency hashes verified against exported
indexedtree ddac5d501d4b03af989ed73e6551afe8f8186e2b, no generated sources.
Transport diagnostic204816bytes SHA256
`9aa988c81c39baee4a5e4165a864abfa74feabed951da0f6c6ac6474a1dfe6f3`.
Its include parses this owner; its existing main does NOT call the pump.
Separate syntax-only probe calls bind/step/close using the real pinned NDK Exec
signatures and MsgPort ownership fields, PASS. Probe was compiled from candidate
build/dev; saved here with exact digest and compiler/flags. Neither probe nor
this candidate was executed. Guard built, not executed.

Next: finite native actual-Wait/termination/retained-close diagnostic, distinct
exact-candidate coordination with live shared guards and explicit release.
Earlier priority5 emulator PASS applies only to its older bytes and changed
context. Default-priority FAILED evidence remains unresolved. No frontend,
sustained audible/timing/physical acceptance. Physical OFF/deferred/unprobed.
