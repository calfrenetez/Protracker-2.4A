# Native validation gates (not run)

Use the repository shared-harness onboarding and a fresh, explicitly coordinated
AmiConnect window. This document prepares acceptance; it is not transport
clearance. Never reuse a historical emulator PID, Safari session, reservation or
recovery result. Do not retarget another task's connection.

1. Pin the exact selected `PT24GEdit`, `PTWorkflowTest`, build manifest and fixture
   inputs. The clean committed-source build and the preserved display-overlay
   build are distinct candidates. Record full SHA256/bytes, actual configuration
   and installed memory. Shared 030 first: 68030, no FPU, AGA, 2 MiB Chip and
   128 MiB Fast. Real A1200/ACA1234 configuration must be read when available.
2. Under the established live guards, run the RAM-only `PTWorkflowTest` with a
   65,536-byte stack. Require RC0, its `WORKFLOW HOST PASS` marker (historical
   fixture wording; on an Amiga it denotes native controller execution), and
   `EXEC MEMORY PASS` with zero owned bytes. This exercises controller calls and
   ordinary Fast memory; it does not qualify remote input or audio hardware.
3. Exercise the actual editor keyboard and two-button mouse. Capture manager,
   toolbox and event-return state; selection stays silent. Check names/IDs,
   frames, formats, loop state, stored-reference and distinct-pattern counts,
   initially unchecked cleanup items, view-only sorting/filtering, stale refresh,
   cancellation and explicit editor/audition controls. Use stored off-order,
   instrument-only, muted and last-supported-track references.
4. Verify no-loop/corrupt-loop/end-at-final-frame handling; preserve zoom, range
   and source bytes. Copy each supported precision/channel format from the
   actual selection, with contained/partial loops and boundary slices. Exercise
   strict free/reserved/configured/unknown ownership, append/255-slot refusal,
   allocation/journal pressure, cancel before publication and one undo/redo.
   Save/reload using existing supported formats; compare decoded music and
   retained masters. No renumbering or source-file deletion is permitted.
5. Verify explicit, empty, instrument-only, portamento and zero-instrument
   inherited events in actual order/control-flow context. Check detached,
   conflicting, unreachable and bounded-unresolved cases, then invalidate the
   resolver through edits. MIDI opens existing routing only and emits nothing.
   Return restores the captured order/pattern/row/track/field without moving
   transport or creating musical undo entries.
6. Measure long-sample overview/copy initial work, step times, total latency,
   cancellation response and peak Fast/Chip usage on the actual 030. Repeated
   selection/status redraw must reuse a completed overview. Repeat open/copy/
   cleanup/undo/redo/history release and verify resource reclamation. Host CPU
   or allocator counters cannot substitute for these observations.
7. In a separately authorized compatible playback run, compare the same retained
   musical content and output before/after the features. Read-only manager,
   selection and viewport/navigation coexistence must add no observable glitches
   or missed deadlines relative to the exact pre-feature build. Audition must
   refuse to steal active voices. Destructive operations require explicit
   Stop+Apply and a positively confirmed release; recording remains protected.
   Keep capture, measured timing and human listening evidence separate.
8. Preserve every failure. Perform exact scoped cleanup, independently verify
   absence and idle return through the harness, then explicitly release the
   window. Release any physical browser control to verified View Only. Stop on
   uncertain ownership/restoration or a new physical failure; do not retry it
   automatically. Physical tests follow the exact emulator qualification.

The missing companion `ACCEPTANCE_TESTS.md` may add required cases. Reconcile it
before declaring completion. Existing unfinished enhanced live output and device
capacity/ordering/completion gates remain open; do not qualify them through this
RAM-only fixture or infer them from a successful editor launch.
