#ifndef PT_SCHEDULED_LINEAGE_H
#define PT_SCHEDULED_LINEAGE_H
#include "scheduled_output.h"
/* Separate opt-in software contract. The legacy flags7 API is unchanged. */
#define PT_LINEAGE_VERSION 1U
#define PT_LINEAGE_CONDITIONAL 1U
#define PT_LINEAGE_ACTIONS 4
struct pt_lineage_output;
struct pt_lineage_key {
    const struct pt_lineage_output *queue;
    uint64_t session,generation,ticket,owner,serial;
    unsigned action,slot;
};
struct pt_lineage_event {
    const struct pt_lineage_output *queue;uint64_t session,owner;
    struct pt_scheduled_event scheduled;
    /* TRIGGER reserves an identity, not activation. CONTROL/STOP target an
     * actually observed original TRIGGER key. Copies, never entry pointers. */
    struct pt_lineage_key key[PT_LINEAGE_ACTIONS];
};
enum pt_lineage_command {
    PT_LINEAGE_WAITING,PT_LINEAGE_ISSUED,PT_LINEAGE_CANCELLED_BEFORE,
    PT_LINEAGE_CANCELLED_AFTER,PT_LINEAGE_FAILED,PT_LINEAGE_UNKNOWN
};
enum pt_lineage_reader {
    PT_LINEAGE_NONE,PT_LINEAGE_ACTIVE,PT_LINEAGE_STOP_PENDING,
    PT_LINEAGE_DRAINING,PT_LINEAGE_RETIRED,PT_LINEAGE_READER_UNKNOWN
};
struct pt_lineage_action_receipt {
    enum pt_lineage_command command;enum pt_lineage_reader reader;
    struct pt_lineage_key key;uint64_t observed,issued;
};
enum pt_lineage_provenance {PT_LINEAGE_BACKEND_ACTUAL,PT_LINEAGE_LOCAL_UNSUBMITTED};
struct pt_lineage_receipt {
    enum pt_lineage_provenance provenance;
    const struct pt_lineage_output *queue;
    uint64_t session,generation,ticket,owner;unsigned count;
    struct pt_lineage_action_receipt action[PT_LINEAGE_ACTIONS];
};
enum pt_lineage_reply {
    PT_LINEAGE_UNCERTAIN=-1,PT_LINEAGE_PENDING=0,
    PT_LINEAGE_OBSERVATION=1,PT_LINEAGE_ALL_RETIRED=2
};
/* All callbacks are serialized bounded task work, never an IRQ entry. Context
 * names its full control extent. Session is nonzero and caller-never-reused.
 * Version/lineage_flags are exact. flags retains exactly REQUIRED7.
 * submit1 atomically accepts a future complete event,0 refuses with no refs,
 * otherwise uncertain. Backend validates actual clock/generation and every
 * target at publication AND original [first,last) activation, before ANY action.
 * Changed target -> whole batch zero effects, never whichever reader owns slot.
 * Observation is actual command/reader history, not predicted time or retirement.
 * ALL_RETIRED confirms this exact ticket has no future activation, DMA reader or
 * callback referencing ANY event/owner span, even shared cache capacities.
 * Cancelling CONTROL before issue need not stop its target; those refs remain
 * until no-reader proof. Partial/unknown execution is a contract failure.
 * Backends cannot edit/reenter borrowed objects, allocate/convert at activation,
 * or represent immediate callbacks as timestamped hardware capability. */
struct pt_lineage_backend {
    void *context;size_t context_bytes;struct pt_scheduled_caps caps;
    unsigned version,lineage_flags;
    int (*read_clock)(void *,uint64_t *,uint32_t *);
    int (*submit)(void *,const struct pt_lineage_event *);
    enum pt_lineage_reply (*poll)(void *,uint64_t,struct pt_lineage_receipt *);
    enum pt_lineage_reply (*cancel)(void *,uint64_t,struct pt_lineage_receipt *);
};
struct pt_lineage_owner {
    struct pt_scheduled_owner held;
    /* Optional terminal classification notification before exactly-once release.
     * Correct identity + independently valid ALL_RETIRED can release despite
     * bad action/timing reports; valid==0 marks that failure. No reentry/free. */
    void (*terminal)(void *,uint64_t,const struct pt_lineage_receipt *,int valid);
};
/* Fixed<=8 entries,<=4 actions, one per slot. Original grid arithmetic retained.
 * All borrowed contexts/owners/queue handles outlive close; no copied/reused
 * handles. Allocator context extents retain caller disjointness contract. */
enum pt_scheduled_result pt_lineage_open(const struct pt_allocator *,
    const struct pt_scheduled_grid *,uint64_t session,const struct pt_lineage_backend *,
    unsigned capacity,struct pt_lineage_output **);
/* target has exactly batch.count elements (NULL only all TRIGGER); TRIGGER
 * elements must be zero. CONTROL/STOP require a genuine ACTIVE same queue key,
 * earlier frame and original TRIGGER slot. STOP closes subsequent descendants
 * conservatively even when later cancelled. Atomic whole-batch refusal leaves
 * queue/owner/ticket/input images unchanged. Owner transfers only on OK. */
enum pt_scheduled_result pt_lineage_enqueue(struct pt_lineage_output *,
    const struct pt_scheduled_batch *,const struct pt_lineage_key *,
    const struct pt_lineage_owner *,uint64_t *);
enum pt_scheduled_result pt_lineage_publish(struct pt_lineage_output *,uint64_t);
/* Each calls at most one backend callback. Pending/uncertain preserve out.
 * Observation copies a validated snapshot but never releases. Retirement
 * independently binds the exact ticket before release; malformed identity
 * retains. Malformed actions/timing with valid retirement latch failure and
 * release once, preserving out. Invalid output alias/wrap calls have no effects.
 * Active observation only is not permission to free owner storage. */
enum pt_scheduled_result pt_lineage_poll(struct pt_lineage_output *,uint64_t,
    struct pt_lineage_receipt *);
enum pt_scheduled_result pt_lineage_cancel(struct pt_lineage_output *,uint64_t,
    struct pt_lineage_receipt *);
/* Read-only key from a registered positively ACTIVE original TRIGGER, never
 * acceptance/cache identity/absence of retirement. Snapshot can become stale;
 * backend must compare again atomically at the scheduled boundary. */
enum pt_scheduled_result pt_lineage_reader_key(struct pt_lineage_output *,
    uint64_t,unsigned,struct pt_lineage_key *);
/* Permanently closes publication; one cancellation per submitted entry/call.
 * Unsubmitted releases locally. Failure/pending retains uncertain live owners.
 * No automatic retry, reset, force unpin or implicit close cancellation. */
enum pt_scheduled_result pt_lineage_stop(struct pt_lineage_output *);
unsigned pt_lineage_held(const struct pt_lineage_output *);
int pt_lineage_close(struct pt_lineage_output *);
#endif
