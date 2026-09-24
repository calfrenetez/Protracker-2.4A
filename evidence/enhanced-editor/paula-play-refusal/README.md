# Active playback preservation on refused requests

2026-09-24: three host Paula sanitizer groups pass; full pinned native build
passes (includes unrelated dirty display work, not release/UI qualification).
Shared030 rerun paula-cache-1790228673542536000 passes all five binaries with
return code0 and all four Paula DMA channels off after cleanup. New regression
checks Studio, AmiGUS, MIDI, mixed routes and genuine24-bit requests against an
active classic replay. Ticks continue; cache pointers/version and audio lock
remain; separate enhanced PCM and sample descriptors are unchanged.

Original run paula-cache-1790228555461654000 is preserved as failed: native
markers and shutdown were present, but the harness observed paula.rc before
Echo populated it. The runner now waits for a trailing completion marker.
Fresh coordination and live guards preceded the rerun; no reset/relaunch. Both
owned runs completed exact cleanup, and each window was explicitly released.

CIA fallback execution is NOT RUN because Workbench owns CIAA timer B; its
existing vector was preserved. These are emulator counters/register/ownership
checks, not listening confirmation or physical A1200/AmiGUS acceptance.
