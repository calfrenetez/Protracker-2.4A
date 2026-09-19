# Enhanced editor dev6 evidence

Binary SHA-256: `0f8e0bcdfea5ebd887c3d10c0f00b2aba79f556f4b06d45572bc3ebbd27ab053`.

All three retained native runs use this exact build. Block commands were exercised
through native Intuition keyboard input and native input.device mouse events.
The block run compares saved bytes and CRC against independent copy/transpose/clone
expectations, checks undo/redo and boundary refusal, reloads/resaves identically,
and plays a block-transposed classic MOD at period 404 with DMA off after Stop.
The requester run covers blank start, load/save dialogs, cancellation/invalid
load preservation, new-file refusal and playback after reopening.

Host tests include a full 1,024-event block in one journal command and compare
incremental/whole-image output through changing selections and secondary panels.
The main-screen golden hash remains unchanged. Historical audio ownership/effect
checks keep their dev4 identity; this build does not repeat that whole suite.
No physical hardware or complete AmiConnect browser transport is certified.
