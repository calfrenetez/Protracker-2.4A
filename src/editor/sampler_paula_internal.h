#ifndef PT_SAMPLER_PAULA_INTERNAL_H
#define PT_SAMPLER_PAULA_INTERNAL_H
#include "sampler_paula.h"
/* Prepared immutable project/master path only. Zero-init caller job, no copying
 * or reinitializing while active. Caller has fully validated project/PCM and
 * holds the genuine current master pin; this API is not an input validator.
 * Begin checks metadata/current token and reserves one budgeted unpublished Chip
 * copy without bulk conversion. HIT transfers a lease immediately. PENDING
 * retains its own master reference. Each step packs/copies <=256 output bytes;
 * publication/transferred output happens only after the final chunk. Allocation,
 * eviction and metadata checks remain synchronous; no hard latency guarantee.
 * Cancel/error releases the unpublished lease and master reference. Caller keeps
 * owner/project/sampler/storage alive through cancellation, and keeps all source
 * arrays immutable. No reentry, direct cache access, mutation or reader output.
 * Close can refuse while a job owns a lease; cancel before disposing contexts.
 * Outputs and job must be disjoint from source/metadata. Source identity/revision
 * changes refuse, but arbitrary in-place value writes are forbidden, not scanned.
 * Successful result lease requires ordinary confirmed reader stop before unpin. */
struct pt_sampler_paula_job {
    struct pt_sampler_paula *owner;struct pt_sampler *sampler;struct pt_project *project,header;
    struct pt_sample_version *pin;struct pt_pcm pcm;struct pt_playback_upload_job upload;
    uint64_t version;unsigned generation,track,sample;
    uint8_t staging[PT_PLAYBACK_UPLOAD_CHUNK];
};
enum pt_cache_result pt_sampler_paula_job_begin(struct pt_sampler_paula_job *,
    struct pt_sampler_paula *,unsigned track,unsigned sample,unsigned source_channel,
    struct pt_sample_version *expected,struct pt_cache_lease *);
enum pt_cache_result pt_sampler_paula_job_step(struct pt_sampler_paula_job *,struct pt_cache_lease *);
void pt_sampler_paula_job_cancel(struct pt_sampler_paula_job *);
#endif
