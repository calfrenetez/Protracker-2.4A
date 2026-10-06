#ifndef PT_MIXED_READERS_ACTIVATION_H
#define PT_MIXED_READERS_ACTIVATION_H
#include "mixed_scheduled_readers.h"
#define PT_MIXED_ACTIVATION_COMMANDS 2U
#define PT_MIXED_ACTIVATION_READERS 32U
#define PT_MIXED_ACTIVATION_SLOTS 20U
#define PT_MIXED_ACTIVATION_PORT_VERSION 1U
#define PT_MIXED_ACTIVATION_PORT_REQUIRED 15U
struct pt_mixed_readers_activation;
/* Opaque identity only. In particular the queue may already have been freed
 * when source_close receives its copied identity. Never dereference either
 * pointer. Session is fresh across owner/queue address recycling. */
struct pt_mixed_activation_registration {
    const struct pt_mixed_readers_activation *owner;
    const struct pt_mixed_readers_output *queue;
    uint64_t session,generation;
};
struct pt_mixed_activation_command_identity {
    struct pt_mixed_activation_registration registration;
    uint64_t ticket,owner;
    const struct pt_mixed_readers_event *event;
    struct pt_mixed_readers_binding binding;
};
struct pt_mixed_activation_reader_identity {
    struct pt_mixed_activation_registration registration;
    struct pt_mixed_readers_key key;
    const struct pt_mixed_readers_domain *reference;
    struct pt_mixed_readers_binding binding;
};
/* Complete copied activation values; no event/domain/span/holder traversal.
 * Queue/binding/cache/reservation addresses are echoed opaque identities.
 * Paula data references independently retained signed8 storage. AmiGUS card
 * ranges are numeric, NEVER CPU spans or discovered physical capacity.
 * All twenty actual original keys (including untouched slots) precede effects.
 */
struct pt_mixed_activation_packet {
    struct pt_mixed_activation_registration registration;
    uint64_t ticket,frame,first,last;
    unsigned count,expected_mask;
    struct pt_mixed_readers_key expected[PT_MIXED_ACTIVATION_SLOTS];
    struct pt_mixed_readers_key key[PT_MIXED_READERS_ACTIONS];
    struct pt_mixed_readers_action action[PT_MIXED_READERS_ACTIONS];
    struct pt_mixed_readers_card card[PT_MIXED_READERS_ACTIONS];
};
struct pt_mixed_activation_actual {
    unsigned active_mask,adopted_mask;
    struct pt_mixed_readers_key slot[PT_MIXED_ACTIVATION_SLOTS];
};
/* Separately injected port, never an immediate two-route wrapper. All entries
 * require caller-owned whole-entry task/activation exclusion; no concurrency,
 * input edits or callback reentry. Flags are declarations, not qualification.
 * read_clock: bounded honest actual clock/frequency, task AND fire pre/post.
 * publish: atomically COPY the entire paired packet and callback owner/ticket
 * identity, before original first; 1 accepted,0 no references/effects,-1 unknown.
 * A clean0 is absence only with unchanged packet/identity and no NEW callback
 * exclusion fault. Raw0 from a faulting/reentrant call is recorded as actual0
 * but classified uncertain, retaining all possible refs for later exact proofs.
 * No retained packet pointer. A key-changing waiting batch cannot be predicted.
 * commit: repeat every actual key before any effect, one whole-paired operation,
 * return1 only actual persistent reference adoption + resulting slot registry.
 * No allocation, conversion, queue/event/domain/span/holder/editor traversal,
 * owner callbacks, early effects, shifted windows or task-wakeup fallback.
 * 0 no effects; -1/partial effects unknown and retain all possible references.
 * quiet: bounded task-only probe/cancel,1 exact independent domain quiet,
 * 0 pending,-1 unknown. Reader cancel disables whole pending batches that could
 * reference that exact reader, preserving replacements. Before reader quiet1,
 * drop every sample/card/geometry reference; copied historical identity alone
 * cannot revive it. Command quiet says nothing about persistent readers.
 * A newly faulting/reentrant or identity-mutating probe retains both domains,
 * even when its actual callback returned1. A later explicit unchanged exact
 * proof may drain previously latched failure; no probe is repeated here.
 * source_close: at most ONE shutdown attempt after actual queue consumption;
 * 1 independently proves
 * no future callback names this exact registration (including owner storage).
 * 0 pending,-1 unknown; retain contexts and the diagnostic, never repeat shutdown.
 * source_quiet: subsequent explicit close calls perform <=one bounded read-only
 * probe of this exact copied registration;1 proves actual source quiet,0/-1/
 * malformed retain storage. A later independent proof can drain uncertainty.
 * No automatic probe, shutdown retry or recovery is performed here.
 * Native IRQ/residency/WCET/stack, paired physical atomicity, card completion,
 * stop and timing require separate evidence; a finite software body proves none.
 */
struct pt_mixed_activation_port {
    void *context;size_t context_bytes;unsigned version,flags;
    int (*read_clock)(void *,uint64_t *,uint32_t *);
    int (*publish)(void *,struct pt_mixed_readers_activation *,const struct pt_mixed_activation_packet *);
    int (*commit)(void *,const struct pt_mixed_activation_packet *,struct pt_mixed_activation_actual *);
    int (*command_quiet)(void *,const struct pt_mixed_activation_command_identity *,unsigned);
    int (*reader_quiet)(void *,const struct pt_mixed_activation_reader_identity *,unsigned);
    int (*source_close)(void *,const struct pt_mixed_activation_registration *);
    int (*source_quiet)(void *,const struct pt_mixed_activation_registration *);
};
struct pt_mixed_activation_config {
    struct pt_allocator allocator;struct pt_mixed_readers_span allocator_context;
    struct pt_mixed_readers_grid grid;uint64_t session;
    size_t control_budget,queue_budget;
    struct pt_mixed_activation_port port;
};
enum pt_mixed_activation_result {PT_MIXED_ACTIVATION_INVALID=-2,
    PT_MIXED_ACTIVATION_FAILED=-1,PT_MIXED_ACTIVATION_EARLY=0,
    PT_MIXED_ACTIVATION_COMMITTED=1};
/* Two fixed ordinary allocations: owner + its genuine typed queue. Complete
 * config/workspace/output/context/new allocation spans guarded before writes
 * or callbacks. Known aliases never initialized or released as fresh storage.
 * Fresh zero workspace of queried alignment/capacity; expires after open.
 * Queue constructor scratch is a disjoint segment of this external workspace;
 * its config/output are local, never embedded in the backend context.
 * If unexpected constructor queue cleanup refuses without consuming its slot,
 * BACKEND publishes a retained recovery owner in *out; preserve it and contexts.
 * Other failed construction attempts preserve *out.
 */
size_t pt_mixed_activation_workspace_size(void);
size_t pt_mixed_activation_workspace_alignment(void);
size_t pt_mixed_activation_control_size(void);
enum pt_mixed_readers_result pt_mixed_activation_open(const struct pt_mixed_activation_config *,
    void *,size_t,struct pt_mixed_readers_activation **);
/* One genuine owned queue, borrowed only. NEVER raw-close it, export its backend,
 * create a second queue from that backend, or dispose owner storage yourself.
 * Factory/adapters may invoke core enqueue/key/services under their complete
 * guards. The extra opaque port context must also be caller-disjoint (core only
 * knows the owner context), or use the guarded wrappers below. Borrowers and
 * every factory handle/pool close before owner close; their untransferred
 * controls are not proved quiet by empty queue/ledger counts.
 * Guarded wrappers pass the actual caller output through the core's full
 * incoming/live-source guards; no local-ticket/receipt bypass. Actual enqueue
 * success transfers ownership even if a holder also faults the activation
 * owner; its ticket remains published and later activation fails closed.
 */
enum pt_mixed_readers_result pt_mixed_activation_borrow_queue(struct pt_mixed_readers_activation *,
    struct pt_mixed_readers_output **);
enum pt_mixed_readers_result pt_mixed_activation_enqueue(struct pt_mixed_readers_activation *,
    const struct pt_mixed_readers_inputs *,uint64_t *);
enum pt_mixed_readers_result pt_mixed_activation_publish(struct pt_mixed_readers_activation *,uint64_t);
enum pt_mixed_readers_result pt_mixed_activation_service_command(struct pt_mixed_readers_activation *,
    uint64_t,unsigned,struct pt_mixed_readers_command_receipt *);
enum pt_mixed_readers_result pt_mixed_activation_service_reader(struct pt_mixed_readers_activation *,
    uint64_t,unsigned,unsigned,struct pt_mixed_readers_reader_receipt *);
enum pt_mixed_readers_result pt_mixed_activation_reader_key(struct pt_mixed_readers_activation *,
    uint64_t,unsigned,struct pt_mixed_readers_key *);
enum pt_mixed_readers_result pt_mixed_activation_stop(struct pt_mixed_readers_activation *);
/* Task-side veto for a fault observed by an enclosing guarded controller.
 * Caller must hold whole-entry task/activation exclusion and positively know
 * that this is its still-live genuine owner. NULL is a no-op. This only latches
 * failure, advances its fault serial and notifies an existing release latch;
 * no queue walk, clock, callback, cancellation, release or hardware effect.
 * May be called within a serialized task-port callback while busy/task_busy;
 * never from an IRQ, a concurrent caller or after actual owner consumption.
 * It prevents later fire, but proves no command/reader/source quiet and never
 * changes an actual callback reply or transfers ownership. */
void pt_mixed_activation_fail_closed(struct pt_mixed_readers_activation *);
/* Independently scheduled exact-ticket invocation, never an automatic timer.
 * Fire traverses copied values only; early preserves original invocation,
 * late/uncertainty/reentry latch failure without erasing possible effects. */
enum pt_mixed_activation_result pt_mixed_activation_fire(struct pt_mixed_readers_activation *,uint64_t);
/* No implicit cancellation/STOP/service. Actual queue close is called with a
 * disjoint LOCAL slot, and internal registration releases only on consumption
 * toNULL, even close0 after release reentry. Nonempty queue refuses unchanged.
 * Final owner release additionally requires exact positive source shutdown.
 * Pending/unknown shutdown retains contexts; only an explicit independent
 * source_quiet probe can subsequently drain it, never another shutdown attempt.
 * NULL-slot close is terminal/idempotent and reads no expired former controls.
 */
int pt_mixed_activation_close(struct pt_mixed_readers_activation **);
#endif
