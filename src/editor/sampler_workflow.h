#ifndef PT_SAMPLER_WORKFLOW_H
#define PT_SAMPLER_WORKFLOW_H
#include "sampler.h"
#include "../core/sample_usage.h"
struct pt_sampler_workflow;
struct pt_sampler_workflow_stats {
    unsigned affected_slots,slots_freed,destination_slot,appended;
    uint64_t retained_master_bytes;
    size_t staged_bytes,master_bytes_released; /* Immediate release is zero: undo retains masters. */
};
/* Explicit stopped=1 certifies transport/recording/affected previews already
 * quiescent through their existing lifecycle. No silent Stop, backend calls,
 * cache eviction, source promotion, history purge, allocation in commit, or IDs/
 * sample_count change for cleanup. Owner/project/history/context and original
 * borrowed project tables/PCM/markers outlive cancel/commit. Captured extents
 * protect outputs while stale; they do not authorize using freed storage.
 * Begin does metadata-only project-span preflight and uses a completed usage
 * snapshot; it does not read PCM values. Semantic project validation is a private
 * first step phase before any PCM copy. Borrowed data stays immutable throughout
 * preparation; every in-place edit must change history revision or sampler
 * generation. Cursor-only channel selection is permitted. Initial fixed metadata,
 * selected-slot allocation and copy marker planning remain synchronous/finite.
 * Begin preflights selected eligible slots, budget
 * and a free journal command. Each job is unique/noncopyable/owner-thread only.
 * Invalid/empty selection, full journal, stale or allocation failure leaves all
 * project/history/current pointers unchanged. Jobs charge the sampler budget.
 * Inputs remain immutable for each call; callbacks may change project/history
 * metadata only with the corresponding revision/generation. Such changes refuse
 * the original transaction rather than adopting new source/target identities.
 * Allocations must be fresh/disjoint. Opaque allocator contexts and outputs must
 * be mutually disjoint; their extents cannot be inferred by this API.
 */
enum pt_edit_result pt_sampler_cleanup_begin(struct pt_sampler *,struct pt_project *,
    struct pt_pattern_history *,const struct pt_sample_usage_preview *,
    const uint8_t selected[PT_PROJECT_SAMPLES],unsigned stopped,
    struct pt_sampler_workflow **);
/* Actual half-open source frame range. Preserve precision/channels/rate/name/
 * volume/finetune/interpolation; translate an entirely contained loop, otherwise
 * disable it. Keep/translate only markers in [start,end). Independent PCM.
 * First genuinely free existing slot; append only when no free slot and <255.
 * Target/source identities and usage are rechecked before one undo resource.
 */
enum pt_edit_result pt_sampler_copy_begin(struct pt_sampler *,struct pt_project *,
    struct pt_pattern_history *,const struct pt_sample_usage_preview *,unsigned source_slot,
    uint32_t start,uint32_t end,unsigned stopped,struct pt_sampler_workflow **);
/* Validation performs <=4096 bounded work items per call, including PCM values,
 * stored events and slice markers. Later copy calls initialize <=4096 PCM/marker
 * bytes. Validation and copying never run in the same call. No publication.
 * Errors retain an unpublished job for explicit cancel; ready unchanged on
 * refusal. No callbacks except the budgeted allocator/release during begin/cancel.
 */
enum pt_edit_result pt_sampler_workflow_step(struct pt_sampler_workflow *,unsigned *ready);
/* Fresh stopped=1 is required again before commit. One chronological resource;
 * entire selection commits/undoes/redoes atomically. The exact completed/current
 * validation is checked again without rescanning PCM. Initial apply consumes
 * that private state; it cannot be reused after publication. Every apply needing
 * free-slot protection makes one synchronous pass over all stored events into
 * a fixed255-slot map, then checks the entire selection before any mutation.
 * This is bounded by project limits, not a native wall-clock guarantee.
 * Only success transfers job ownership to history and clears *job. Stats are
 * published on success only and do not promise cache/card/undo reclamation.
 */
enum pt_edit_result pt_sampler_workflow_commit(struct pt_sampler_workflow **,unsigned stopped,
    struct pt_sampler_workflow_stats *);
void pt_sampler_workflow_cancel(struct pt_sampler_workflow **);
#endif
