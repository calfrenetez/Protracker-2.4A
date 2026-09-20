# Channel details and lossless export guard, dev21

CHANNEL > DETAILS adds hexadecimal pan 00–FF, group 00–0F, decimal MIDI channel
1–16 and a 15-character track name. Return applies one chronological undo command;
Escape cancels, including preserving redo. Keyboard/mouse input cannot change the
selected channel while a value is being entered. BACK returns to routing controls.
The original main-screen golden remains unchanged.

All 29 host groups pass. Sanitized controller tests cover bounds, empty/invalid
values, hexadecimal versus decimal input, cancelled and cleared names, 15-character
limit, modal channel isolation, cross-property undo/redo and byte-exact PTG
metadata persistence. Full and incremental planar rendering agree through details,
number/name proposals, commit/cancel, undo/redo and closing. A separate strict-MOD
regression proves that changing only a Paula track's MIDI assignment is refused
without writing output; resetting the assignment restores exact MOD export.

Native Intuition input applies PAN 80, GROUP 0F, MIDI 16 and BASS on channel 1,
rejects MIDI 17, preserves redo through cancelled name entry, and names channel 5
PAD. Independent expected-byte construction verifies the entire saved PTG,
including CRC and unchanged unrelated song bytes. Reopen/resave is byte-identical.
The normal-MOD workflow refuses a non-default MIDI assignment before opening a
requester; undo returns clean. Native PTModProjectTest repeats the direct-export
regression and PTPaulaTest validates names/groups/dormant MIDI assignment during
initial and live Paula playback, strict disk-export refusal and unchanged source
metadata. Enhanced pan still refuses/stops this classic backend. Its existing
replay/resource-ownership regression also passes; CIAA fallback execution remains
unavailable where Workbench owns that vector, as explicitly recorded in the log.

A first channel-details UI run passed before the additional native replay fix.
The retained final workflow repeats all UI/file checks using the final binaries
and includes the expanded replay test. Screenshots are native emulator output.
All three editor sessions exit normally and audio DMA is off at completion.

Group is saved organisational metadata; group stems, AmiGUS panning and CAMD
output are not connected by this change. Pan is retained without conversion;
Paula keeps its hardware stereo placement. Physical A1200/AmiGUS testing is NOT
RUN. Emulator ownership remains reserved for coordinated ProTracker validation.
