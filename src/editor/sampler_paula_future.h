#ifndef PT_SAMPLER_PAULA_FUTURE_H
#define PT_SAMPLER_PAULA_FUTURE_H
#include "sampler.h"
#include "../core/scheduled_output.h"
#define PT_PAULA_FUTURE_OWNERS 8
#define PT_PAULA_FUTURE_SOURCES 4
struct pt_paula_future_pool;
struct pt_paula_future_owner;
enum pt_paula_future_result {PT_FUTURE_OK,PT_FUTURE_PENDING,PT_FUTURE_INVALID,
    PT_FUTURE_CAPACITY,PT_FUTURE_STALE,PT_FUTURE_BUSY};
struct pt_paula_future_config {
    unsigned maximum_owners;size_t control_budget,chip_budget;uint64_t generation;
    void *chip_context;void *(*chip_allocate)(void *,size_t);
    void (*chip_release)(void *,void *,size_t);
};
/* NULL previous starts a new trigger source. A retained retrigger source references a genuine
 * transferred-live owner, its never-reused token and source index. It must use
 * the same queue, earlier frame and stable slot. No STOP lineage is accepted.
 * No source storage may change in-place; use sampler edits/generation instead. */
struct pt_paula_future_request {
    unsigned track,sample,channel;
    struct pt_paula_future_owner *previous;uint64_t token;unsigned source;
};
struct pt_paula_future_view {
    const uint8_t *data;size_t bytes;uint32_t frames;
    unsigned track,sample,channel,slot;uint64_t token;
};
/* Task-side preparation only. Bind validates synchronously; allocation/eviction
 * at begin remain synchronous. Pool allocation + each owner are budgeted apart
 * from sampler master storage and the dedicated optional Chip budget. No IRQ,
 * DMA, output adapter, voice allocation or hardware capability is implemented.
 * Borrowed project, sampler, allocators, queue and inputs stay alive/immutable.
 * Outputs/input control descriptors must additionally be disjoint from the
 * borrowed opaque queue and allocator/Chip callback contexts, whose complete
 * extents this API cannot discover. No copying/reinitializing handles, reentry,
 * source mutation or releasing/reinitializing borrowed objects before close. */
enum pt_paula_future_result pt_paula_future_open(const struct pt_allocator *,
    struct pt_sampler *,struct pt_project *,const struct pt_paula_future_config *,
    struct pt_paula_future_pool **);
enum pt_paula_future_result pt_paula_future_begin(struct pt_paula_future_pool *,
    struct pt_scheduled_output *,uint64_t,const struct pt_paula_future_request *,unsigned,
    struct pt_paula_future_owner **);
/* One transition OR <=4096 master bytes OR <=256 Chip bytes per call. No
 * partially prepared source view/cache is exposed. Valid errors cancel partial
 * resources; invalid output aliases preserve outputs and the owner/job state. */
enum pt_paula_future_result pt_paula_future_step(struct pt_paula_future_owner *,unsigned *ready);
enum pt_paula_future_result pt_paula_future_view(struct pt_paula_future_owner *,unsigned,
    struct pt_paula_future_view *);
/* TRIGGER only, one action per requested stable slot, count equals source count.
 * Each must be an aligned word range of its actual current signed8 lease.
 * CONTROL/STOP always refuse: resource lineage proves no active-reader state.
 * Batch generation/frame must match begin. Whole-batch refusal
 * preserves owner/queue/output; transfer occurs ONLY on actual queue OK.
 * All queue use is serialized with pool use, including backend callbacks. */
enum pt_scheduled_result pt_paula_future_enqueue(struct pt_paula_future_owner *,
    const struct pt_scheduled_batch *,uint64_t *);
/* Transferred-live cancellation/close refuses. Only queue all-reader retirement
 * releases refs; a predecessor with a preparation borrower defers its own ref
 * release until the borrower acquires independent exact pins/HIT lease or
 * cancels. Handles remain until explicit close. Retirement is per event. */
enum pt_paula_future_result pt_paula_future_cancel(struct pt_paula_future_owner *);
int pt_paula_future_owner_close(struct pt_paula_future_owner *);
/* Refuses with state unchanged until all owners closed; never force unpins. */
int pt_paula_future_close(struct pt_paula_future_pool *);
#endif
