#ifndef PT_SAMPLER_SONG_H
#define PT_SAMPLER_SONG_H
#include "sampler_studio.h"
#include "../core/studio_song.h"
struct pt_sampler_song;
/* Owner-thread bridge: captures sampler generation and constructs slot+1 master
 * bindings. Allocator, sampler and project objects must outlive close. Stop BEFORE
 * any edit/undo, document replacement, history/sampler release or reinitialization.
 * This helper does not intercept editor actions. It additionally fails closed on
 * changed sampler generation or replaced project tables at the next pull; this
 * is not permission to free owner objects or mutate patterns during playback.
 * Initial master promotion may allocate from sampler budget, preserving precision.
 * Failure leaves *out unchanged; owned allocations are unwound. */
enum pt_render_result pt_sampler_song_open(struct pt_sampler *,struct pt_project *,
    const struct pt_render_options *,const struct pt_allocator *,struct pt_sampler_song **out);
enum pt_render_result pt_sampler_song_pull(struct pt_sampler_song *,unsigned,
    const struct pt_pcm **,unsigned *done);
void pt_sampler_song_stop(struct pt_sampler_song *);
void pt_sampler_song_close(struct pt_sampler_song *);
#endif
