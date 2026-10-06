# Optional editor mixed preparation session

`editor_mixed_prepare_session` connects the existing cancellable master
establishment and checked preparation paths for a task-side caller. It preserves
authoritative sampler-owned 8/16/24-bit masters and exact musical scheduling.
The accepted classic layout and native PLAY entry are unchanged.

The caller supplies an attached editor binding, separate zeroed establishment,
bridge and checked controls, an initially zeroed session, device metadata,
render options and an immutable original input record. One stable ordinary
context extent contains the complete session and every non-NULL allocator/device/
backend callback context; the other controls remain outside that extent. Complete
callback subobject extents and residency are caller obligations. Keep all named
storage and the original inputs alive through confirmed close.

1. `begin` guards all controls before the first metadata allocation and starts
   the existing master establishment job.
2. Each `advance` does one establishment step, one confirmed-stop/metadata-bind/
   checked-adoption handoff without yielding, or one checked preparation call.
   The existing `work`1..4096 bound applies to items/bytes; wall-clock and stack
   limits remain unmeasured.
3. `READY` means checked software preparation. This API does not create a timer
   or transport, activate output, upload device samples or certify hardware.
4. `close` makes one explicit ownership barrier attempt. A refused drain or
   retained allocator child keeps the remaining hooks and contexts alive.
   A later explicit attempt can finish after that condition is resolved.

Failure/cancellation latch and advances cannot restart the session. Actual editor
changes and disposal use the existing attached binding barrier. A failed checked
admission still retains both idle bound engines behind the session's finish hook;
owner close, both engine closes and the checked zero-child finish precede closure.
Completed masters remain sampler-owned. Terminal get/close/advance read only the
finished session, allowing former input/control storage to expire after close1.

Host ASan/UBSan qualification passed three groups:102 session lifecycle cases,
78 admission/control/context aliases and3 expired-terminal cases across8/16/24,
plus unchanged checked, establishment and owner regressions. Actual edit/undo/
dispose, exact save preservation, reentry, cancellation, allocation refusals,
both independent drains and retained children are covered. The new preparation
body asserts zero voice starts, timer calls and sample-bus uploads.
See [saved host evidence](../evidence/enhanced-editor/editor-prepare-session/README.md).

Its exact full fixture compiles/links for m68000/softfloat/nix20,395508 bytes;
see [portable compiler evidence](../evidence/enhanced-editor/editor-prepare-session-portable/README.md).
The exact source86 fixture subsequently passed once in Amiberry: all three
692-byte stdout markers, RC0 and COMPLETE in343.511 seconds under the unchanged
420-second limit. It executed the full portable assertion bodies with injected
callbacks on68030/no-FPU/AGA. Separate completed settlement and independent release
confirmed all2386 pins, retained six-file custody, original disconnected endpoint,
process/listener absence and EMPTY108. All three peers were explicitly released.
See [saved emulator evidence](../evidence/enhanced-editor/editor-prepare-session-native/README.md).
The real A1200 has not yet run this coordinator. Earlier physical qualification
covers the separate frozen editor-bridge fixture. Native UI/event-loop/PLAY integration, real backend cache
completion/stop and exact scheduled activation remain unfinished. No physical
allocation placement, total stack, device capacity/order/completion, IRQ, audio
or listening acceptance is inferred from the host tests.
