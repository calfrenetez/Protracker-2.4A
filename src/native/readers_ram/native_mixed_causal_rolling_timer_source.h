/* Separate fixed four-event HOST resource model. No native IRQ ABI/MMIO.
 * Two recyclable logical slots do not imply physical CIA counters. */
#ifndef PT_PRIVATE_MIXED_CAUSAL_ROLLING_TIMER_SOURCE_H
#define PT_PRIVATE_MIXED_CAUSAL_ROLLING_TIMER_SOURCE_H
#ifndef PT_PRIVATE_ROLLING_TIMER_HOST_RESOURCE_MODEL
#error "Explicit rolling HOST resource-model opt-in required"
#endif
#include "native_mixed_causal_pair_timer_source.h"
#define PT_PRIVATE_ROLLING_TIMER_EVENTS 4U
struct pt_private_rolling_timer_event {
    struct pt_private_pair_timer_event original;
    unsigned slot;
};
struct pt_private_mixed_causal_rolling_timer_source {
    struct pt_private_mixed_causal_rolling_timer_source *self;
    struct pt_private_pair_timer_ops ops;
    struct pt_private_mixed_causal_ram_port *port;
    struct pt_mixed_causal_registration registration;
    struct pt_private_pair_timer_hw_state before;
    struct pt_private_rolling_timer_event event[PT_PRIVATE_ROLLING_TIMER_EVENTS];
    unsigned slot_event[2]; /* Original record index + 1, never a caller ticket. */
    uint64_t cookie,exclusion_token;
    unsigned initialized,bound,acquire_attempted,acquired,acquire_absent,acquire_unknown,baseline_invalid;
    unsigned scope,entry_busy,ops_busy,irq_active,dispatch_active,exclusion_unknown;
    unsigned exclusion_restores,exclusion_refusals;
    unsigned arms_seen,failed,faults,source_attempted,source_closed;
    unsigned irq_entries,dispatches,task_entries,quiet_probes;
    int acquire_outcome,source_outcome,last_dispatch;
};
/* Fresh full zero, finite noncopyable caller storage. Exactly four original
 * attempts; no reset/refill. The existing typed rolling RAM/core/factory must
 * independently retire and dispose an old command before another publication.
 * This source adds its own exact-ticket independent quiet plus hardware absence;
 * a numerical count, absent pending bit or old NULL pointer is not that proof.
 * All original event identities remain copied values even after slot reuse.
 * Source/port/ops storage and code remain live through independent SOURCE quiet
 * AND full outer task_leave/exclusion restoration. No sample metadata or consumed
 * owner/queue controls are dereferenced by the source. Existing pair is unchanged.
 * All resource primitive contracts are exactly the shared pair API contracts. */
int pt_private_rolling_timer_init(struct pt_private_mixed_causal_rolling_timer_source *,
    const struct pt_private_pair_timer_ops *);
int pt_private_rolling_timer_task_enter(struct pt_private_mixed_causal_rolling_timer_source *);
int pt_private_rolling_timer_task_leave(struct pt_private_mixed_causal_rolling_timer_source *);
/* Once under whole task exclusion after genuine EMPTY rolling RAM bind, before
 * factory packets/source exposure. Pair/control/finite ports are refused. */
int pt_private_rolling_timer_bind(struct pt_private_mixed_causal_rolling_timer_source *,
    struct pt_private_mixed_causal_ram_port *,const struct pt_mixed_causal_registration *);
struct pt_private_mixed_causal_ram_adapter pt_private_rolling_timer_adapter(
    struct pt_private_mixed_causal_rolling_timer_source *);
/* Ticket-free vector: actual pending slot selects retained original identity.
 * EARLY rearms that same absolute deadline. Stale delivery fails without ack or
 * cancellation of the current reused slot. HOST-only, no timing/native claim. */
int pt_private_rolling_timer_irq(void *);
#endif
