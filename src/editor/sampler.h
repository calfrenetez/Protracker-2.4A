#ifndef PT_SAMPLER_H
#define PT_SAMPLER_H
#include "pattern.h"
#include "document.h"
#include "slices.h"
#include "svx.h"
struct pt_sample_version;
struct pt_sampler {
    struct pt_sample_version *current[PT_PROJECT_SAMPLES];
    struct pt_allocator allocator;
    size_t bytes,budget;
    unsigned generation;
    pt_pcm_progress progress;
    void *progress_context;
};
void pt_sampler_init(struct pt_sampler *,const struct pt_allocator *,size_t);
/* Release journal first, then sampler, before destroying/reinitializing editor.
   Versions are immutable and must never be edited through project pointers. */
void pt_sampler_release(struct pt_sampler *);
enum pt_edit_result pt_sampler_import(struct pt_sampler *,struct pt_project *,struct pt_pattern_history *,unsigned,const uint8_t *,size_t,const char *);
enum pt_edit_result pt_sampler_edit(struct pt_sampler *,struct pt_project *,struct pt_pattern_history *,unsigned,enum pt_pcm_edit,uint32_t,uint32_t,unsigned);
/* Loop metadata and baked crossfade are each a single shared undo command.
   Baking produces a forward loop with the consumed head skipped. */
enum pt_edit_result pt_sampler_loop(struct pt_sampler *,struct pt_project *,struct pt_pattern_history *,unsigned,enum pt_loop_kind,uint32_t,uint32_t,uint32_t);
/* Replace non-destructive markers. Never silently retarget a referenced ordinal. */
enum pt_edit_result pt_sampler_slices(struct pt_sampler *,struct pt_project *,struct pt_pattern_history *,unsigned,const uint32_t *,size_t);
/* Explicit whole-sample conversion. Linear rate conversion preserves marker
   ordinals by scaling frames; collapsed markers/invalid loop geometry refuse. */
enum pt_edit_result pt_sampler_convert(struct pt_sampler *,struct pt_project *,struct pt_pattern_history *,unsigned,unsigned,uint32_t);
enum pt_edit_result pt_sampler_convert_quality(struct pt_sampler *,struct pt_project *,struct pt_pattern_history *,unsigned,unsigned,uint32_t,unsigned);
/* Explicit IFF export preserves PCM/name/rate/volume/forward loop. Unsupported
   precision, stereo, loop kinds, slices and finetune refuse without conversion. */
enum pt_svx_result pt_sampler_svx_size(const struct pt_sample *,size_t *);
enum pt_svx_result pt_sampler_svx_encode(const struct pt_sample *,uint8_t *,size_t,size_t *);
#endif
