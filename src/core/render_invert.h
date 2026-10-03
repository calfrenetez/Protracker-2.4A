#ifndef PT_RENDER_INVERT_H
#define PT_RENDER_INVERT_H
#include "render.h"
/* Explicit offline classic EFx path. sample_budget bounds all additional private
 * sample descriptors and PCM; ordinary renderer workspace uses the same caller
 * allocator separately. All channels retain shared mutation clocks regardless of
 * audio selection/mute/solo. Requires whole mono8 one-shots or forward loops
 * (at least four loop frames), and no interpolation/slices. Private one-shots
 * start with a cleared first word and retain the classic two-frame DMA repeat,
 * even when EFx makes it audible; original master bytes are preserved.
 * Unknown/unsupported inputs fail before sink calls. Masters are never edited.
 * This does not enable EFx in the queued Studio/hardware sequence API. */
enum pt_render_result pt_render_invert_stream(const struct pt_project *,const struct pt_render_options *,
    pt_render_sink,void *,pt_render_progress,void *,struct pt_render_report *,
    size_t sample_budget,const struct pt_allocator *);
/* Incremental private-bank producer for an output queue:48kHz stereo24 only.
 * Same bounded classic input subset/budget as offline rendering. Controller and
 * sequencer allocations also use a, separately from the private-sample budget.
 * Project metadata/events/orders/allocator must remain immutable and live until
 * stop/close. Caller must stop before editing or replacing a document; this core
 * does not provide sampler generation checks. Never mutates or pins masters.
 * Success owns a private bank; failure preserves *out and frees all allocations.
 * This opt-in core API does not enable native PLAY or existing Studio entry points. */
struct pt_render_invert_session;
enum pt_render_result pt_render_invert_open(const struct pt_project *,const struct pt_render_options *,
    size_t sample_budget,const struct pt_allocator *,struct pt_render_invert_session **);
/* Cancellable equivalent: begin owns bounded storage after synchronous input
 * validation/static selection/allocation, but copies no PCM and does not measure
 * the complete timeline. Each prepare initializes one copy job, copies <=4096
 * bytes, completes the private bank, or measures <=256 timeline ticks. Only
 * ready=1 permits pull; pending pull refuses without advancing and emits no PCM.
 * Close/stop cancels any phase. Preparation errors are sticky and release private
 * state; the session still needs close. No output callbacks/pins/queue blocks.
 * Options copied; project/source metadata and PCM must stay immutable and live
 * through close. Bounds are progress limits, not native timing/device evidence.
 * The synchronous open wrapper drives these phases to full readiness. */
enum pt_render_result pt_render_invert_begin(const struct pt_project *,const struct pt_render_options *,
    size_t sample_budget,const struct pt_allocator *,struct pt_render_invert_session **);
enum pt_render_result pt_render_invert_prepare(struct pt_render_invert_session *,unsigned *ready);
/* Owner-wrapper output guard, before any caller-output write. Checks bounded
 * metadata spans, not PCM values or generations. Live inputs retain validated
 * geometry; stopped sessions inspect their controller alone, without former
 * project/storage dereferences. NULL session/output refuses. */
int pt_render_invert_output_disjoint(const struct pt_render_invert_session *,const void *,size_t);
/* One bounded interval transition OR <=256 audio frames per pull. NULL audio and
 * done=0 means progress; yield then pull again. PCM is borrowed until next pull,
 * stop or close: pumps/queues must copy it. Stop/end/error release all private
 * voices and samples. A consumer-held queue copy remains independent and must
 * still be drained/released. Internal errors poison the session; no retry. No
 * allocations during pull; end/stop/error call allocator release. Owner-thread
 * only, no timing/device claim. */
enum pt_render_result pt_render_invert_pull(struct pt_render_invert_session *,unsigned max_frames,
    const struct pt_pcm **,unsigned *done);
void pt_render_invert_stop(struct pt_render_invert_session *);
void pt_render_invert_close(struct pt_render_invert_session *);
#endif
