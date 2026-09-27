#ifndef PT_SAMPLER_INTERNAL_H
#define PT_SAMPLER_INTERNAL_H
#include "sampler.h"
#define PT_SAMPLER_PIN_CHUNK 4096
/* Private preparation job for an ALREADY fully validated immutable project.
 * Zero-initialize, serialize with sampler/editor, cancel before mutations or
 * owner/document release. All contexts/source storage must outlive cancellation.
 * Begin performs bounded metadata checks and one synchronous budgeted allocation,
 * with no bulk copy. Step copies at most bytes (1..4096), checking generation,
 * table/current-version and exact descriptor identity before each chunk. In-place
 * source writes are forbidden, not detected. This is not an input validator.
 * Publication is atomic after the entire PCM/marker copy. Prior output PCM/token
 * remain unchanged until ready=1; errors preserve outputs and poison stale jobs.
 * Cancel releases reserved bytes without changing the project. Success transfers
 * one pin to caller; unpin exactly once. Public sampler_pin remains validating.
 * Job storage must not alias inputs/outputs; no copying or reinitializing a job. */
struct pt_sampler_pin_job {
    struct pt_sampler *owner;struct pt_project *project;struct pt_sample *table;
    struct pt_sample_version *value,*previous;struct pt_sample source;
    size_t values,copied_values,copied_slices;
    unsigned slot,count,generation;enum pt_edit_result failure;
};
enum pt_edit_result pt_sampler_pin_job_begin(struct pt_sampler_pin_job *,struct pt_sampler *,struct pt_project *,unsigned,unsigned);
enum pt_edit_result pt_sampler_pin_job_step(struct pt_sampler_pin_job *,size_t,struct pt_pcm *,struct pt_sample_version **,unsigned *ready);
void pt_sampler_pin_job_cancel(struct pt_sampler_pin_job *);
#endif
