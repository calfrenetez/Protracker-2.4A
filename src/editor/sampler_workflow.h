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
 * Initial project/PCM validation is synchronous. Begin preflights a validated
 * usage snapshot, selected eligible slots, budget
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
/* At most4096 PCM/marker bytes initialized per call. No playable publication.
 * Errors retain an unpublished job for explicit cancel; ready unchanged on
 * refusal. No callbacks except the budgeted allocator/release during begin/cancel.
 */
enum pt_edit_result pt_sampler_workflow_step(struct pt_sampler_workflow *,unsigned *ready);
/* Fresh stopped=1 is required again before commit. One chronological resource;
 * entire selection commits/undoes/redoes atomically.
 * Only success transfers job ownership to history and clears *job. Stats are
 * published on success only and do not promise cache/card/undo reclamation.
 */
enum pt_edit_result pt_sampler_workflow_commit(struct pt_sampler_workflow **,unsigned stopped,
    struct pt_sampler_workflow_stats *);
void pt_sampler_workflow_cancel(struct pt_sampler_workflow **);
#endif
