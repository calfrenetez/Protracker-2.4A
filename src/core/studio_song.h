#ifndef PT_STUDIO_SONG_H
#define PT_STUDIO_SONG_H
#include "studio_plan.h"
struct pt_studio_song;
/* Owner-thread, immutable-project song session, 48k stereo24 only. Bindings are
 * copied, exactly one per project sample in slot order, matching its descriptor.
 * Source/allocator contexts and project/PCM must outlive close; stop before edits
 * or document replacement. Provider must pin the exact bound master generation.
 * Three bounded allocations (controller, sequencer, mixer), no output on failure.
 * Open drives begin/prepare synchronously. Failure preserves *out and releases all state.
 * Handle outputs must be disjoint from live input argument structures, project
 * tables/PCM capacities and owned session storage. Copied argument structures
 * need not outlive begin/open; this adds no lifetime requirement for them. */
enum pt_render_result pt_studio_song_open(const struct pt_project *,const struct pt_render_options *,
    const struct pt_allocator *,const struct pt_studio_source *,const struct pt_studio_binding *,unsigned,
    struct pt_studio_song **out);
/* Begin validates inputs synchronously, then prepare advances one measurement
 * chunk (<=256 ticks), silent interval transition or phase chunk (<=256 frames).
 * No source callbacks, pins or audio during preparation. Project/source storage
 * stays immutable and alive; stop/close cancels. Pull refuses until ready=1.
 * Initial validation, allocation and final sequence reset remain synchronous.
 * This bounds traversal, not elapsed time. Preparation failure poisons session. */
enum pt_render_result pt_studio_song_begin(const struct pt_project *,const struct pt_render_options *,
    const struct pt_allocator *,const struct pt_studio_source *,const struct pt_studio_binding *,unsigned,
    struct pt_studio_song **out);
enum pt_render_result pt_studio_song_prepare(struct pt_studio_song *,unsigned *ready);
/* Metadata-only output guard for owner wrappers. No PCM-value reads, callbacks
 * or allocation. Live sessions protect project/table/master capacities and owned
 * storage. After stop only the surviving controller is inspected, never former
 * borrowed project/tables. Validated borrowed geometry must remain immutable.
 * Prepare refuses alias/stopped/failed calls before modifying caller readiness. */
int pt_studio_song_output_disjoint(const struct pt_studio_song *,const void *,size_t);
/* Only after readiness: copy 255 slot flags (including pre-roll and repeat
 * sources). Refusal leaves output unchanged. No ownership transfer. */
int pt_studio_song_required(const struct pt_studio_song *,uint8_t samples[255]);
/* Pull at most max_frames (1..256). Performs at most one interval transition OR
 * one audio block, including discarded pre-roll. OK + *pcm=NULL + *done=0 means
 * progress without output: yield/control-check then pull again. Non-null PCM is
 * session-owned, valid only until next pull/stop/close; caller must copy to queues.
 * OK + done=1 marks end/stopped, with no output. Natural end and internal failure
 * release mixer pins and sequence allocations. Error poisons playback; no retry.
 * Invalid/overlapping outputs are refused unchanged without advancing or source
 * callbacks. PCM/done outputs must be mutually disjoint and outside all live
 * project/table/master capacities and owned storage (including the previous block).
 * No transport, interrupt
 * safety or real-time guarantee. Provider callbacks may allocate while dispatching. */
enum pt_render_result pt_studio_song_pull(struct pt_studio_song *,unsigned max_frames,
    const struct pt_pcm **pcm,unsigned *done);
void pt_studio_song_stop(struct pt_studio_song *);
void pt_studio_song_close(struct pt_studio_song *);
#endif
