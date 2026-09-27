#ifndef PT_SAMPLER_SONG_H
#define PT_SAMPLER_SONG_H
#include "sampler_studio.h"
#include "../core/studio_song.h"
struct pt_sampler_song;
/* Owner-thread bridge: captures sampler generation and constructs slot+1 master
 * bindings. Allocator, sampler and project objects must outlive close. Stop BEFORE
 * any edit/undo, document replacement, history/sampler release or reinitialization.
 * This helper does not intercept editor actions. It additionally fails closed on
 * changed sampler generation or project header at the next prepare/pull; this
 * is not permission to free owner objects or mutate patterns during playback.
 * Open prepares all used masters synchronously, preserving precision.
 * Failure leaves *out unchanged; owned allocations are unwound. */
enum pt_render_result pt_sampler_song_open(struct pt_sampler *,struct pt_project *,
    const struct pt_render_options *,const struct pt_allocator *,struct pt_sampler_song **out);
/* Cancellable preparation: no output before whole-sequence analysis and all
 * required masters are pinned (including pre-roll/repeat sources). Each prepare
 * performs one analysis step, one budgeted allocation, or <=4096 PCM/marker bytes
 * copied. Unused samples are not promoted. Initial/reset validation and allocation
 * remain synchronous. Full source-provider validation remains at playback acquire.
 * Pull during preparation advances one step and returns NULL PCM/done=0 until
 * ready; errors poison/stop. Stop releases jobs/pins; unchanged completed masters
 * may remain sampler-owned. Cancel before edits or releasing any borrowed storage. */
enum pt_render_result pt_sampler_song_begin(struct pt_sampler *,struct pt_project *,
    const struct pt_render_options *,const struct pt_allocator *,struct pt_sampler_song **out);
enum pt_render_result pt_sampler_song_prepare(struct pt_sampler_song *,unsigned *ready);
enum pt_render_result pt_sampler_song_pull(struct pt_sampler_song *,unsigned,
    const struct pt_pcm **,unsigned *done);
void pt_sampler_song_stop(struct pt_sampler_song *);
void pt_sampler_song_close(struct pt_sampler_song *);
#endif
