#ifndef PT_SAMPLER_LIFETIME_INTERNAL_H
#define PT_SAMPLER_LIFETIME_INTERNAL_H
#include "sampler.h"
/* Intrinsic owner-thread bookkeeping only. Never copy live nodes or enroll
 * caller ownership. These private leaves grant no playback, registry, mutation
 * or native authority. Ordinary lifetime construction has no fixed node limit;
 * only the enhanced-domain metadata check below is bounded to4096. */
#define PT_SAMPLER_LIVE_CHECK_LIMIT 4096U
enum pt_sampler_live_kind {
    PT_LIVE_VERSION=1,PT_LIVE_CHANGE,PT_LIVE_APPEND,PT_LIVE_SLOT,
    PT_LIVE_WORKFLOW,PT_LIVE_TABLE,PT_LIVE_ESTABLISH
};
enum pt_sampler_pending_kind {
    PT_PENDING_PIN=1,PT_PENDING_WORKFLOW,PT_PENDING_ESTABLISH,
    PT_PENDING_CALL
};
struct pt_sampler_live_span {const void *data;size_t bytes;};
struct pt_sampler_live {
    struct pt_sampler_live *previous,*next;
    struct pt_sampler *owner;
    struct pt_sampler_live_span spans[3];
    unsigned count,kind;
};
struct pt_sampler_pending {
    struct pt_sampler_pending *previous,*next;
    struct pt_sampler *owner;
    const void *control,*project;
    size_t bytes;
    unsigned kind,slot,generation;
    struct pt_sampler_live_span retiring[3];unsigned retiring_count;
};
size_t pt_sampler_table_allocation_bytes(void);
struct pt_sample *pt_sampler_table_allocate(struct pt_sampler *);
void pt_sampler_table_release(struct pt_sampler *,struct pt_sample *);
int pt_sampler_live_add(struct pt_sampler *,struct pt_sampler_live *,unsigned,
    const void *,size_t,const void *,size_t,const void *,size_t);
int pt_sampler_live_remove(struct pt_sampler_live *);
int pt_sampler_pending_add(struct pt_sampler *,struct pt_sampler_pending *,
    unsigned,const void *,size_t,const void *,unsigned,unsigned);
int pt_sampler_pending_retire(struct pt_sampler_pending *,const struct pt_sampler_live *);
int pt_sampler_pending_remove(struct pt_sampler_pending *);
int pt_sampler_pending_transfer(struct pt_sampler_pending *,struct pt_sampler_pending *,
    const void *,size_t);
/* Pure bounded numeric metadata checks, not a persistent ready certificate.
 * Unknown/inconsistent/overlimit inventory refuses; no allocation/callback/output.
 * Every named owner/context and original live node must remain alive/immutable
 * for the call. No values or replacement pointers are followed. */
int pt_sampler_lifetime_idle(const struct pt_sampler *);
/* Intrinsic storage inventory status only: fault-free empty live/pending
 * bookkeeping, no charged bytes, current versions or table obligations.
 * Idle alone is insufficient: pins may retain versions after sampler_release.
 * This is side-effect-free owner-thread metadata, not document release authority
 * or owner-factory membership. The sampler must stay alive and immutable for
 * this call; no pointed-to storage is followed and no readiness is retained. */
int pt_sampler_lifetime_storage_retired(const struct pt_sampler *);
int pt_sampler_lifetime_output_disjoint(const struct pt_sampler *,const void *,size_t);
#endif
