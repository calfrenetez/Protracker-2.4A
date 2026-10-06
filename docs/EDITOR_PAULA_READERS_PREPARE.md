# Optional editor Paula reader preparation

`editor_paula_readers_prepare` is an opt-in host software controller for the
actual attached editor. It composes genuine all-slot 8/16/24-bit master
establishment with the existing opaque Paula song producer and ABI2 queue.
It preserves the original musical grid, session and absolute frame schedule.
No classic PLAY, layout, native UI or event-loop wiring is added.

Supply a genuine attached `pt_editor_mixed` binding, a separate zeroed
`pt_editor_mixed_establish`, a zeroed single-use controller, immutable original
inputs and ordinary construction scratch using the producer's actual size and
alignment queries. A complete composite context extent contains the controller
and every allocator, Chip and backend callback context. Establishment, editor,
binding and source capacities remain disjoint from that extent; original inputs
may reside within it when disjoint from the controller and callback context
extents. Unknown opaque extents and addressable residency remain caller duties.
Keep all named storage alive and immutable until `close` returns1.

`begin` guards complete controls, contexts, source capacities and the whole
declared scratch capacity before writes or allocator callbacks. Each `step`
does one existing establishment step, one serialized confirmed-establishment-stop
and producer-construction handoff, or one existing producer step. In the handoff,
the private preparation close hook is installed before the producer's first
allocation. No owner cast, copied checked owner or public READY certificate is
used. `work`1..4096 retains the existing item/byte bound; finite metadata checks
do not establish wall-clock, placement or aggregate-stack limits.

A bounded 66-slot allocator extent registry guards the whole producer and all
live ordinary and Chip allocations. It is an alias guard; the genuine producer
and sampler retain ownership and source-proof authority. Allocation aliases are
not released as fresh ownership. Original callback presence is checked before
wrapping. Fixed original editor/project identity and headers precede source-table
walks. Status and receipt outputs use complete pre/post guards and stay unchanged
after a failed call. Terminal getters, step and close avoid expired controls.

Publication is explicit: one original clock read and at most one submit attempt.
Each explicit command or reader service performs at most one corresponding
backend callback. Effect callbacks retain their actual accepted, uncertain or
confirmed-refusal classifications; controller faults do not invent release
proofs. Reserved readers, WAIT_ACTIVE and pressure do not rebase timestamps.
Empty DONE sends no musical STOP; the optional explicit terminal STOP retains
the original terminal frame and exact prospective keys. There is no timer
opening/service or immediate voice-start fallback.

Failure and cancellation latch; this controller never retries, rebuilds or
forces release. Actual editor change, undo or disposal uses the attached veto
barrier: cancel local producer work without musical STOP or receipt polling,
then attempt close. Submitted or uncertain command and reader domains retain
the hook and veto mutation until distinct exact proofs drain through explicit
services. Use NULL receipt outputs for stale-source draining without former
table reads. A producer close can return0 after consuming its owner and setting
the slot to NULL; the controller retains its hook for a distinct later explicit
close attempt. Completed masters remain sampler-owned and authoritative, and
the tests preserve exact saved project bytes.

Saved HOST_ONLY ASan/UBSan qualification passes six compile/run calls with empty
stderr. New coverage includes 72 actual precision-specific lifetimes, 81 full
admissions, 24 ABI2 publication/reentry/independent-drain cases, three complete
oracle song schedules including WAIT_ACTIVE/pressure/original terminal STOP,
three complete output-alias groups, three poisoned former-table cases and three
expired-terminal cases. The inherited producer, caller-workspace and actual
editor establishment regressions also pass. See the
[saved host evidence](../evidence/enhanced-editor/editor-paula-readers-prepare/README.md).

The separate [portable build evidence](../evidence/enhanced-editor/editor-paula-readers-prepare-portable/README.md)
records a corrected compiler/link PASS and preserves its original compiler
refusal. The candidate's native entry and test constructor remain NOT_RUN.
This new controller's target execution, native UI/PLAY integration, real backend,
MMIO/DMA, placement, aggregate stack, device capacity/order/completion/stop,
exact activation, task/IRQ timing, physical audio and listening gates remain open.
