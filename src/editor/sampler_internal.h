#ifndef PT_SAMPLER_INTERNAL_H
#define PT_SAMPLER_INTERNAL_H
#include "sampler.h"
/* Retain a genuine already-held immutable pin for another voice. The session
 * has fully validated its project and keeps source arrays immutable. This checks
 * generation, current-token/owner and exact descriptor identity without scanning
 * PCM/markers or allocating. expected must stay pinned through this call; this is
 * not an input validator. Success transfers one additional reference; unpin once.
 * Refusal leaves both outputs unchanged. Owner-thread, non-reentrant only. */
enum pt_edit_result pt_sampler_pin_current(struct pt_sampler *,struct pt_project *,unsigned slot,
    unsigned generation,struct pt_sample_version *expected,struct pt_pcm *,struct pt_sample_version **);
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
/* Metadata-only output guards for live immutable sampler ownership. No PCM
 * value scans, allocation or callbacks. Caller checks project/generation first,
 * retains every version/job, and never calls after those owners are released.
 * Version accounting bytes may cover separate adopted PCM; inspect actual
 * header, capacity, markers and flat backing spans separately. NULL version is
 * an empty span; other invalid/overflowing arguments refuse. */
int pt_sampler_version_output_disjoint(const struct pt_sample_version *,const void *,size_t);
int pt_sampler_output_disjoint(const struct pt_sampler *,const void *,size_t);
int pt_sampler_pin_job_output_disjoint(const struct pt_sampler_pin_job *,const void *,size_t);
/* Enumerate genuine held version storage without exposing its private layout.
 * Metadata only; <=6 header/PCM/marker spans including a flat backing. Outputs
 * are unchanged on refusal and must be disjoint from every named source span
 * and each other. Zero-length spans are omitted. Sampler/context stays alive. */
#define PT_SAMPLER_VERSION_SPANS 6
struct pt_sampler_storage_span {const void *data;size_t bytes;};
int pt_sampler_version_spans(const struct pt_sample_version *,
    struct pt_sampler_storage_span *,unsigned,unsigned *);
#endif
