#ifndef PT_SAMPLER_INVERT_SONG_H
#define PT_SAMPLER_INVERT_SONG_H
#include "sampler.h"
#include "../core/render_invert.h"
struct pt_sampler_invert_song;
/* Explicit private-bank EFx producer. Same classic subset as render_invert; no
 * source precision conversion or immutable master pins. Sampler/project objects
 * and metadata must outlive stop. Stop BEFORE edit/undo, document replacement,
 * history/sampler release or reinitialization. Prepare/pull fail closed on changed
 * sampler generation or any project header byte except channels.selected; the
 * fixed header snapshot does not copy pointed-to tables/PCM or authorize owner
 * destruction or in-place pattern writes while active. No editor guard/device is installed.
 * Uses a for controller/private bank; sample_budget bounds the private bank.
 * Failure leaves *out unchanged and releases every producer allocation. */
/* Begin performs bounded initial allocation/static validation. Prepare advances
 * one core private-copy/setup step (at most4096 PCM bytes) or at most256 measured
 * ticks, and publishes ready=1 only after complete preparation. Static scans and
 * allocation remain synchronous; these work bounds are not timing guarantees.
 * Failure is sticky and frees private state; close still releases the wrapper.
 * Pull while pending advances one prepare step with NULL PCM/done=0. Stop may
 * cancel any phase. Existing open drains begin/prepare synchronously. Prepare
 * refuses readiness outputs overlapping live producer/owner/project/PCM storage
 * before mutation. Invalid, stale, stopped and already-failed calls preserve the
 * output; stopped/failed calls do not inspect formerly borrowed tables. Begin/open
 * also refuse external handle output aliases before publication and unwind all
 * allocations without changing the output or any borrowed source storage. */
enum pt_render_result pt_sampler_invert_song_begin(struct pt_sampler *,struct pt_project *,
    const struct pt_render_options *,size_t sample_budget,const struct pt_allocator *,
    struct pt_sampler_invert_song **out);
enum pt_render_result pt_sampler_invert_song_prepare(struct pt_sampler_invert_song *,unsigned *ready);
enum pt_render_result pt_sampler_invert_song_open(struct pt_sampler *,struct pt_project *,
    const struct pt_render_options *,size_t sample_budget,const struct pt_allocator *,
    struct pt_sampler_invert_song **out);
enum pt_render_result pt_sampler_invert_song_pull(struct pt_sampler_invert_song *,unsigned,
    const struct pt_pcm **,unsigned *done);
/* Metadata-only owner/editor forwarding, with no former-table traversal after
 * Stop/failure or lost generation/project identity. Stale/already-failed pull preserves
 * both caller outputs; alias refusal never advances the private producer. */
int pt_sampler_invert_song_matches_owner(const struct pt_sampler_invert_song *,const struct pt_sampler *,const struct pt_project *);
int pt_sampler_invert_song_output_disjoint(struct pt_sampler_invert_song *,const void *,size_t);
void pt_sampler_invert_song_stop(struct pt_sampler_invert_song *);
void pt_sampler_invert_song_close(struct pt_sampler_invert_song *);
#endif
