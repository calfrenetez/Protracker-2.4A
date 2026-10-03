#ifndef PT_SAMPLER_PAULA_LINEAGE_H
#define PT_SAMPLER_PAULA_LINEAGE_H
#include "sampler.h"
#include "../core/scheduled_lineage.h"
#define PT_PAULA_LINEAGE_OWNERS 8
#define PT_PAULA_LINEAGE_SOURCES 4
struct pt_paula_lineage_pool;
struct pt_paula_lineage_owner;
enum pt_paula_lineage_result {PT_PAULA_LINEAGE_OK,PT_PAULA_LINEAGE_PENDING,PT_PAULA_LINEAGE_INVALID,
    PT_PAULA_LINEAGE_CAPACITY,PT_PAULA_LINEAGE_STALE,PT_PAULA_LINEAGE_BUSY};
struct pt_paula_lineage_config {
    unsigned maximum_owners;size_t control_budget,chip_budget;uint64_t generation;
    void *chip_context;void *(*chip_allocate)(void *,size_t);
    void (*chip_release)(void *,void *,size_t);
};
/* TRIGGER has zero key/no previous. CONTROL/STOP requires an actual positively
 * ACTIVE original TRIGGER from this pool and exact same lineage queue/session.
 * previous/token/source borrow only preparation metadata; never active proof.
 * Its original source must match track/sample/channel and stable routed slot. */
struct pt_paula_lineage_request {
    enum pt_scheduled_kind kind;unsigned track,sample,channel;
    struct pt_paula_lineage_owner *previous;uint64_t token;unsigned source;
    struct pt_lineage_key key;
};
struct pt_paula_lineage_view {
    const uint8_t *data;size_t bytes;uint32_t frames;
    unsigned track,sample,channel,slot;uint64_t token;
};
/* Separate bounded pool/cache. No legacy future/prepared owner sharing/casts.
 * Borrowed project/sampler/allocator/queue stay alive and immutable through close.
 * No copied handles, in-place source edits, reentry, IRQ/DMA/output integration.
 * Outputs/control inputs also remain disjoint from opaque queue/backend control and allocator/
 * Chip callback contexts whose complete extents this API cannot discover. */
enum pt_paula_lineage_result pt_paula_lineage_open(const struct pt_allocator *,
    struct pt_sampler *,struct pt_project *,const struct pt_paula_lineage_config *,
    struct pt_paula_lineage_pool **);
enum pt_paula_lineage_result pt_paula_lineage_begin(struct pt_paula_lineage_pool *,
    struct pt_lineage_output *,uint64_t,const struct pt_paula_lineage_request *,unsigned,
    struct pt_paula_lineage_owner **);
/* One transition OR <=4096 master bytes OR <=256 Chip bytes. No partial cache or
 * view. CONTROL/STOP obtains independent exact-current master and identical
 * serial cache refs, then severs raw predecessor borrow before READY. Target key
 * is rechecked each step and before enqueue; same-cache identity is not activation.
 * Invalid aliases preserve state/output; valid stale/errors cancel partial refs. */
enum pt_paula_lineage_result pt_paula_lineage_step(struct pt_paula_lineage_owner *,unsigned *ready);
enum pt_paula_lineage_result pt_paula_lineage_view(struct pt_paula_lineage_owner *,unsigned,
    struct pt_paula_lineage_view *);
/* One action per request/stable slot, exact frame/generation/kind. TRIGGER only
 * addresses actual retained signed8 geometry; CONTROL/STOP has no data/words.
 * Whole-batch refusal preserves caller ownership/input/output. Transfer only OK.
 * Current callbacks validate resources only; core/backend revalidate full target
 * set at publication and actual activation. No recursive queue key callback. */
enum pt_scheduled_result pt_paula_lineage_enqueue(struct pt_paula_lineage_owner *,
    const struct pt_scheduled_batch *,uint64_t *);
enum pt_scheduled_result pt_paula_lineage_reader_key(struct pt_paula_lineage_owner *,unsigned,
    struct pt_lineage_key *);
/* LIVE cancel/close refuses. Core per-ticket positive all-reader retirement alone
 * releases refs; unsubmitted local core retirement is explicitly local provenance.
 * Preparation borrowers defer original disposal until independently pinned/cancelled.
 * Handles remain registered until explicit close, tokens never reused. Finite<=8
 * pressure is intentional: command issue/cancel-before is not reader retirement. */
enum pt_paula_lineage_result pt_paula_lineage_cancel(struct pt_paula_lineage_owner *);
int pt_paula_lineage_owner_close(struct pt_paula_lineage_owner *);
int pt_paula_lineage_close(struct pt_paula_lineage_pool *);
#endif
