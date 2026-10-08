/* Private HOST resource-stub proposal. Native IRQ/assembly/entry NOT QUALIFIED.
 * Two logical resource deadlines do not imply two physical CIA counters. */
#ifndef PT_PRIVATE_MIXED_CAUSAL_PAIR_TIMER_SOURCE_H
#define PT_PRIVATE_MIXED_CAUSAL_PAIR_TIMER_SOURCE_H
#include "native_mixed_causal_ram_port.h"
#ifndef PT_PRIVATE_PAIR_TIMER_HOST_RESOURCE_MODEL
#error "Explicit HOST resource-model opt-in required"
#endif
#if defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "Causal pair timer source is HOST-only until native IRQ ABI/callgraph/aperture qualification"
#endif
#define PT_PRIVATE_PAIR_TIMER_OPS_VERSION 1U
#define PT_PRIVATE_PAIR_TIMER_TWO_ABSOLUTE 1U
#define PT_PRIVATE_PAIR_TIMER_IRQ_RESOURCE_OPS 2U
#define PT_PRIVATE_PAIR_TIMER_WHOLE_EXCLUSION 4U
#define PT_PRIVATE_PAIR_TIMER_REQUIRED 7U
#define PT_PRIVATE_PAIR_TIMER_TASK 1U
#define PT_PRIVATE_PAIR_TIMER_IRQ 2U
struct pt_private_mixed_causal_pair_timer_source;
struct pt_private_pair_timer_hw_event {
    uint64_t ticket,first;
    uint32_t frequency;
    unsigned armed,pending,queued;
};
struct pt_private_pair_timer_hw_state {
    void *source;
    int (*entry)(void *);
    uint64_t cookie;
    unsigned available,vector_live,callbacks_inflight,restore_pending;
    uint32_t before_image[8];
    struct pt_private_pair_timer_hw_event event[2];
};
struct pt_private_pair_timer_ops {
    void *context;size_t context_bytes;
    unsigned version,capabilities;
    /* Entire task/IRQ entry exclusion, including all genuine owner calls.
     * enter returns a nonzero token preserving the previous mask/task state.
     * leave synchronously restores that token exactly; neither may allocate,
     * sleep or call task-only services in IRQ scope. These are trusted primitive
     * obligations, not facts established by the HOST stub or capability bits. */
    uint64_t (*exclude)(void *,unsigned);
    void (*restore_exclusion)(void *,unsigned,uint64_t);
    int (*clock)(void *,uint64_t *,uint32_t *);
    int (*state)(void *,struct pt_private_pair_timer_hw_state *);
    /* acquire installs ONLY the supplied source-specific entry and saves the
     * complete before image. 1 owned;0 no NEW effects;otherwise unknown. */
    int (*acquire)(void *,void *,int (*)(void *),
        const struct pt_private_pair_timer_hw_state *,uint64_t *);
    /* Independently retain both logical slots; second cannot replace first.
     * Absolute original first is never rebased. Call/state/ack/cancel/release
     * are bounded IRQ-safe primitive operations under whole exclusion. */
    int (*arm)(void *,uint64_t,unsigned,uint64_t,uint64_t,uint32_t);
    /* Clear this EARLY wake path and retain/rearm its SAME absolute first.
     * This is an IRQ-safe provider obligation, never a later rebased wake. */
    int (*rearm)(void *,uint64_t,unsigned,uint64_t,uint64_t,uint32_t);
    int (*ack)(void *,uint64_t,unsigned,uint64_t);
    int (*cancel)(void *,uint64_t,unsigned,uint64_t);
    int (*release)(void *,void *,uint64_t,const struct pt_private_pair_timer_hw_state *);
};
struct pt_private_pair_timer_event {
    struct pt_mixed_causal_registration registration;
    uint64_t ticket,first;
    uint32_t frequency;
    unsigned known,owned,terminal,quiet,uncertain,cancel_attempted;
    int arm_outcome,fire_outcome,rearm_outcome,ack_outcome,cancel_outcome;
};
struct pt_private_mixed_causal_pair_timer_source {
    struct pt_private_mixed_causal_pair_timer_source *self;
    struct pt_private_pair_timer_ops ops;
    struct pt_private_mixed_causal_ram_port *port;
    struct pt_mixed_causal_registration registration;
    struct pt_private_pair_timer_hw_state before;
    struct pt_private_pair_timer_event event[2];
    uint64_t cookie,exclusion_token;
    unsigned initialized,bound,acquire_attempted,acquired,acquire_absent,acquire_unknown,baseline_invalid;
    unsigned scope,entry_busy,ops_busy,irq_active,dispatch_active,exclusion_unknown;
    unsigned exclusion_restores,exclusion_refusals;
    unsigned arms_seen,failed,faults,source_attempted,source_closed;
    unsigned irq_entries,dispatches,task_entries,quiet_probes;
    int acquire_outcome,source_outcome,last_dispatch;
};
/* Fresh zero, caller-owned finite storage; noncopyable/no reset/refill. Full
 * Original owner controls remain live until actual genuine owner disposal;
 * their identities are opaque after consumption. Source, required port storage
 * and ops context/code outlive independent exact SOURCE quiet AND matching
 * full outer entry/primitive completion and task_leave/exclusion restoration.
 * Positive quiet inside TASK scope cannot free storage needed by that leave.
 * ops/source/port spans must be independent of
 * every owner/queue/control/master/library span before exposure. Bind checks
 * fixed owner/queue controls; it does not inspect sample or library metadata.
 * No queue/owner pointer is dereferenced by this source after consumption. */
int pt_private_pair_timer_init(struct pt_private_mixed_causal_pair_timer_source *,
    const struct pt_private_pair_timer_ops *);
int pt_private_pair_timer_task_enter(struct pt_private_mixed_causal_pair_timer_source *);
int pt_private_pair_timer_task_leave(struct pt_private_mixed_causal_pair_timer_source *);
/* Called under task exclusion immediately after genuine owner/port bind,
 * before factory, packets or source exposure. Includes empty owners. */
int pt_private_pair_timer_bind(struct pt_private_mixed_causal_pair_timer_source *,
    struct pt_private_mixed_causal_ram_port *,const struct pt_mixed_causal_registration *);
struct pt_private_mixed_causal_ram_adapter pt_private_pair_timer_adapter(
    struct pt_private_mixed_causal_pair_timer_source *);
/* Resource vector entry takes NO caller ticket. Actual pending state selects
 * copied original slot/ticket; genuine RAM dispatch decides EARLY/window/failure.
 * The current genuine port's task-only diagnostic is an explicit dependency.
 * Proposed scalar matcher composition also remains native NOT_QUALIFIED. */
int pt_private_pair_timer_irq(void *);
#endif
