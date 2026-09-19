#ifndef PT_SAMPLER_H
#define PT_SAMPLER_H
#include "pattern.h"
#include "document.h"
struct pt_sample_version;
struct pt_sampler {
    struct pt_sample_version *current[PT_PROJECT_SAMPLES];
    struct pt_allocator allocator;
    size_t bytes,budget;
    unsigned generation;
};
void pt_sampler_init(struct pt_sampler *,const struct pt_allocator *,size_t);
/* Release journal first, then sampler, before destroying/reinitializing editor.
   Versions are immutable and must never be edited through project pointers. */
void pt_sampler_release(struct pt_sampler *);
enum pt_edit_result pt_sampler_import(struct pt_sampler *,struct pt_project *,struct pt_pattern_history *,unsigned,const uint8_t *,size_t,const char *);
enum pt_edit_result pt_sampler_edit(struct pt_sampler *,struct pt_project *,struct pt_pattern_history *,unsigned,enum pt_pcm_edit,uint32_t,uint32_t,unsigned);
#endif
