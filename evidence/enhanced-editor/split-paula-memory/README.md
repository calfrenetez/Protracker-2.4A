# Split Paula memory — 2026-09-23

101 host tests PASS169.850s and full pinned native cross-build PASS.
The shared 68030 emulator ran the exact PTPaulaTest hash in result.json.
Native assertions verified independently owned Chip samples, Fast replay
metadata, omission of unused payloads, safe rebuild on changed instruments/PCM,
empty-sample DMA guard and existing playback/ownership/scope behaviour.
Two deliberately oversized 31-sample playback attempts exhausted 2 MiB Chip RAM
partway through allocation. Both cleaned up, preserved source PCM and allowed
normal playback afterward. A 4 KiB free-memory tolerance covers OS bookkeeping.

The adapter supplies explicit sample pointers to the pinned replay initializer;
it retains the original first-word/loop fixes without mutating vendor input or
master PCM. No analogue audio or physical A1200/AmiGUS acceptance is claimed.
CIAA fallback execution was skipped because Workbench owned that vector.
All four DMA channels were off after exit, run-owned files cleaned, lock and
resource released to AmiConnect/infrastructure tasks. Shared guest left running.

The checkout retains unrelated uncommitted display work. PTPaulaTest does not
link it; the full editor does. No clean full-editor release package is claimed.
Cache reuse/LRU across sessions and enhanced sample conversion remain open;
all referenced blocks are pinned for the current session and freed after stop.
