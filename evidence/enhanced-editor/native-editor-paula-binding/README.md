# Prepared native editor binding — host and cross-build PASS

Indexed tree19fda182e548f6e5cf7973a74c7991b21ae17938. Native helper syntax checked
with pinned Amiga compiler,37 repository dependencies identity-verified. Existing
PTExecEditorPaulaTest cross-built208560bytes SHA256
2d3c47989eeaf738af5ac72c481dc719d99ccb22f62905b0c6f936ba3a70260e;
147 compiled source hashes verified, zero generated sources; guard built NOT
executed. Neither binary nor helper executed in the emulator this milestone.

Four ASan/UBSan host suites PASS25.892s: new native prepared binding across8/16/24,
existing editor Paula ownership,24 device output cases and existing engine cases.
New test uses actual song preflight/preparation/staging with mocked Exec/DMA,
preserves declared precision/sample snapshots, cancels before song publication,
retains live Chip storage while modeled DMA remains enabled, and blocks changes,
disposal, detach and restart when songNULL but finalFREE is still pending.
Explicit completion allows cleanup/restart. Capability refusal starts no reader
and closes through the same barrier with bounded explicit retries.

Native helper owns copied options and engine plus attached editor binding;
advance reserves/binds/begins OR performs one preparation call, never starts voices.
Existing prepared song APIs emit output afterward; this fixture uses numerical
progression only, not real-time delivery. Optional post-song release callback
makes the existing editor change barrier cover final device/context cleanup,
including no-song preparation. No UI/PLAY/event-loop/timer installation, native
runtime test, listening/timing/physical or emulator resources claimed.

Intermediate fixture compile/main remapping and expected first silent interval
were corrected before final host checks. A close after capability refusal needs
explicit retry after FREE submission; test now follows this existing contract.
No production behavior was weakened to make those earlier checks pass.
