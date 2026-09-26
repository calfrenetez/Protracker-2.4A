#ifndef PT_SAMPLER_INVERT_SONG_H
#define PT_SAMPLER_INVERT_SONG_H
#include "sampler.h"
#include "../core/render_invert.h"
struct pt_sampler_invert_song;
/* Explicit private-bank EFx producer. Same classic subset as render_invert; no
 * source precision conversion or immutable master pins. Sampler/project objects
 * and metadata must outlive stop. Stop BEFORE edit/undo, document replacement,
 * history/sampler release or reinitialization. Pull additionally fails closed on
 * changed generation/table identity; it does not authorize owner destruction or
 * in-place pattern writes while active. No editor guard/device is installed.
 * Uses a for controller/private bank; sample_budget bounds the private bank.
 * Failure leaves *out unchanged and releases every producer allocation. */
enum pt_render_result pt_sampler_invert_song_open(struct pt_sampler *,struct pt_project *,
    const struct pt_render_options *,size_t sample_budget,const struct pt_allocator *,
    struct pt_sampler_invert_song **out);
enum pt_render_result pt_sampler_invert_song_pull(struct pt_sampler_invert_song *,unsigned,
    const struct pt_pcm **,unsigned *done);
void pt_sampler_invert_song_stop(struct pt_sampler_invert_song *);
void pt_sampler_invert_song_close(struct pt_sampler_invert_song *);
#endif
