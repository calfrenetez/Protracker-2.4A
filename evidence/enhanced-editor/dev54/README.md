# Dev54 instrument-only reference compatibility

Software/emulator milestone PASS; physical acceptance remains open.

Six pinned PTSampleTraceTest fixtures were each captured twice, byte-identically,
with the retained baseline matching its previous trace.13 native executions
completed41.463s. Fields cover output periods/volumes, stored sample ranges,
loop pointers and actual DMA trigger counts. These are event/register traces,
not a physical audio capture. Guarded release22:46:59UTC recorded.

Renderer support allows silent sample preload and same whole-sample reload
without resetting phase. It preserves volume and offset/retrigger ordering.
Active different-sample and sliced handoffs stay refused before any sink output
or report mutation, pending a separate current/pending repeat-source model.

Host parity test passed5.220s: six native-trace-driven PCM cases plus independent
31-frame true24 phase/volume oracle ontrack16, silent preload and atomic refusal
boundaries. An older general renderer test expected instrument preload to fail;
updated to verify actual silent PCM, passing2.206s. Earlier failed full-run log
will be retained separately. Main-screen/editor unchanged in this milestone.

67 host regressions PASS121.337s. Rebuilt native renderer passed10 executions
in 171.084s: independent true24 boundary checks, six native-trace-driven PCM cases,
exact host/native44.1k16 and48k24 CLI WAVs, and active cross-sample handoff refusal
without an output file. Accepted classic main-screen golden unchanged. Guarded
shutdown and fresh release checks recorded; AmiConnect explicitly notified.

Package README was also corrected to describe already-completed finetune,
tremolo, glissando, selected-row and stem support accurately.
