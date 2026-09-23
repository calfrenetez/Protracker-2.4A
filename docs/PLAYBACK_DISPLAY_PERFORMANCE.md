# Enhanced-editor display performance checkpoint

The tested application is `PT24GEdit`, the separate enhanced editor introduced
in 5bd9514. The original-source `PT2.4G` executable retains the classic assembler
editor. Sharing the replay engine and font does not establish equivalent display
behaviour or responsiveness.

## Reference and cause

The pinned 2.3F `vbint` invokes `Scope` every active-screen vertical blank.
`RedrawPattern` prepares all 64 rows in `TextBitplane`; `SetPatternPos` changes the
Copper bitplane pointer to move those prepared rows under a fixed central bar.
It advances by complete musical rows, not interpolated pixels between rows.

The enhanced editor previously used IDCMP_INTUITICKS, two WaitTOF calls after
each redraw, and a full-screen cache invalidation whenever first_row changed.
The physical 7f54aaa trial's closed log shows median presentation intervals of
6 replay ticks before scrolling and 36 during scrolling at 125 BPM. The owner
confirmed movement in IMG_0737.mov but rejected refresh speed and smoothness.

## Changes

- Pace the application with an asynchronous timer.device UNIT_VBLANK request;
  rearm before drawing, wait with window signals, and abort/wait/close on exit.
- Remove the two post-render display waits. Keep CIA replay independent.
- Move already rendered pattern rows and draw exposed/changed rows, rather
  than repainting the entire interface for each scroll step.
- Move already displayed rows using graphics.library ClipBlit and upload only
  exposed/changed rows. Preserve window clipping. Scope presentation writes only
  the two yellow planes, restoring the RastPort mask after each operation.
- Draw scope strokes directly into the two yellow bitplanes over their cleared
  background; redraw tempo and position fields only when their values change.
- Reduce repeated 64-bit modulo operations in scope sampling while preserving
  initial/loop bounds, retriggers, pitch and fractional phase.
- Log per-row updates and aggregate presentation cadence, rather than formatting
  and flushing complete replay/frame diagnostics for every scope refresh.

## Validation and limits

99 host tests passed after the pacing/scroll/scope-phase changes. After the last
scope-rendering refinement, the sanitizer-backed editor and native-view-probe
host tests passed again. The editor test compares full rendering, cached pixels
and presented dirty rectangles across 260 scroll/playback states, both scroll
directions, jumps, wrap, volume changes and tempo changes. The scope test compares
sample indices against direct modulo calculations for arbitrary loop lengths,
periods and elapsed ticks. The unchanged stopped-screen golden still matches.

Pinned native build passed. The owned 68030 emulator passed the render benchmark,
native Paula tests, playback/follow/edit-cursor/stop/exit check and existing sampler
import/edit/undo/export/save/reopen workflow. Emulator evidence lives in the shared
infrastructure result 20260923T141017192320Z and task evidence
playback-fixes-20260923T140918Z.

Physical 68030 offscreen rendering: 24 scroll steps take 34 ticks at 50 Hz with
cached scrolling, versus 629 ticks with full redraws; final pixels are identical.
This is about 18.5 times less CPU rendering time, excluding Chip RAM transfer and
display presentation. It does not establish a corresponding frame-rate speedup.
Physical evidence: task physical-view-20260923T141307Z.

The fixed central playhead and exact 2.3F layout are not implemented by this
performance patch. The enhanced editor still follows the cursor at the edge of
its twenty-row view. Screen mode and flicker are unchanged and deferred by the
owner. Physical visual smoothness is an owner acceptance gate; these benchmarks
alone do not clear it.

## Remaining gate and direction decision

The first physical candidate (b76762d9...) still ran at roughly 7 displayed
updates/sec in sustained scrolling intervals, with up to about 26/sec before
scrolling. It improved on the earlier long stalls but does not meet the intended
classic responsiveness. Both editor instances exited zero; saved/reopened files
were byte-identical; the temporary environment override was removed.

A further ClipBlit/plane-mask candidate passes 50 native graphics-library pixel
comparisons, including both directions, jumps, wrap, dynamic scopes and restored
write masks. Playback/follow/edit-cursor/stop/exit checks also pass in the emulator
(task evidence playback-fixes-20260923T142356Z), but scrolling remains too slow
there. This latest candidate has not been deployed physically. Its benchmarks
are not physical or visual acceptance.

The owner was asked whether to adapt the original 2.3F prepared-pattern display
approach for the enhanced editor (recommended), retaining the new project and
editing cores, or continue optimising the rewritten display. No dependent
architecture change is made while that decision is pending. Fixed-centre scrolling
is still unimplemented; flicker remains deferred.

Graphics API references: [graphics primitives](https://wiki.amigaos.net/wiki/Graphics_Primitives)
documents ClipBlit overlap/layer handling and the BltBitMapRastPort write mask.
