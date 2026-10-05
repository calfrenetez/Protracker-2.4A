#ifndef PT_SAMPLER_ESTABLISH_H
#define PT_SAMPLER_ESTABLISH_H
#include "sampler_master_guard_internal.h"
#define PT_ESTABLISH_PARENTS 16U
enum pt_establish_result {PT_ESTABLISH_PENDING,PT_ESTABLISH_READY,PT_ESTABLISH_INVALID,
    PT_ESTABLISH_CAPACITY,PT_ESTABLISH_STALE,PT_ESTABLISH_SEMANTIC,PT_ESTABLISH_ALIAS,PT_ESTABLISH_FAULT};
struct pt_establish_metadata;
/* Private genuine zero-init caller owner, never copy/edit active fields. BEFORE
 * any borrowed playback/renderer/audit sequence or device reader. Caller serializes
 * sampler/project, keeps original/current source tables/storage and all named
 * controls alive/immutable until cancel; ONLY this job may atomically publish a
 * completed master descriptor. Valid selection cursor alone may move.
 * Begin: metadata-only complete source/control guards, one ordinary bounded
 * metadata allocation; no values/pins/master/cache/backend work. Up to14 caller
 * spans plus allocator argument/parent-vector extents; keep all named extents
 * alive through cancel. Unlisted opaque context extents stay caller-disjoint.
 * Step work1..4096: <=work descriptor copies, OR actual project validation items,
 * OR one budgeted guarded master reservation, OR <=work PCM/marker bytes. The
 * genuine validator is cancelled BEFORE promotion. Current/alias metadata guards
 * are separately finite, not wall-clock bounds. Copies initial+expected sample
 * descriptors and extension metadata so original capacities stay guarded even
 * after descriptors move. Spare capacity is protected, never copied as audio.
 * Exact sampler allocator remains bound; no new master ledger or allocator swap.
 * Completed slots stay sampler-owned on failure/cancel; cancellation is not undo.
 * Partial unpublished version is discarded. READY establishes ALL sample slots,
 * never semantic/backend/timing certification for later playback: downstream
 * established mixed owner still performs its full validation/audit.
 * No in-place/destructive callbacks, live backend calls, cache upload, outputs,
 * schedule changes, native placement/aggregate-stack/latency/IRQ/device acceptance.
 */
struct pt_sampler_establish {
    struct pt_sampler *sampler;
    struct pt_project *project,snapshot;
    struct pt_sampler initial_sampler,expected_sampler;
    struct pt_allocator control_allocator;
    struct pt_sampler_storage_span parents[PT_ESTABLISH_PARENTS];
    struct pt_project_validation validation;
    struct pt_sampler_pin_job job;
    struct pt_establish_metadata *metadata;
    size_t metadata_bytes;
    uint32_t revision,generation;
    unsigned active,busy,faulted,phase,parent_count,copied_samples,copied_extensions,slot;
    enum pt_establish_result result;
};
enum pt_establish_result pt_sampler_establish_begin(struct pt_sampler_establish *,struct pt_sampler *,
    struct pt_project *,const struct pt_allocator *,const struct pt_sampler_storage_span *,unsigned,uint32_t);
enum pt_establish_result pt_sampler_establish_step(struct pt_sampler_establish *,uint32_t,uint32_t,unsigned);
/* Checked status; never a general ready certificate. No output writes. */
enum pt_establish_result pt_sampler_establish_get(struct pt_sampler_establish *,uint32_t,uint32_t);
/* Reads no former source arrays. Reentry latches fault and refuses; retry once
 * outside the original call. Normal cancel is idempotent, ends the source borrow,
 * releases exactly one owned metadata block and any unpublished master job. */
int pt_sampler_establish_cancel(struct pt_sampler_establish *);
#endif
