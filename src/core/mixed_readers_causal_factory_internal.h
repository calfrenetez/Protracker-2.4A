#ifndef PT_MIXED_READERS_CAUSAL_FACTORY_INTERNAL_H
#define PT_MIXED_READERS_CAUSAL_FACTORY_INTERNAL_H
#include "mixed_readers_causal.h"
#include "mixed_scheduled_readers_internal.h"
/* Private task-only original identity check, not READY/quiet/work authority.
 * Actual genuine owner/queue/registration plus empty never-published custody
 * are checked under owner exclusion. No pointer output, callback, clock or
 * source acquisition. Used only before the one genuine factory attempt. */
int pt_mixed_causal_factory_original_empty(struct pt_mixed_causal_owner *,
    struct pt_mixed_readers_output *,uint64_t session,uint64_t generation);
/* Distinct STOP factory authority: the actual immutable installed STOP port,
 * original registration and entirely empty custody are checked under exclusion.
 * A caller flag, copied registration or ordinary trigger-only owner is not a
 * capability. The completed-first query is likewise actual owner state only;
 * it is not a reader key, scheduling readiness or source-quiet certificate.
 * Exclusion may invoke the original port ownership callback; these queries
 * do not invoke source-acquisition or publication callbacks. */
int pt_mixed_causal_factory_stop_original_empty(struct pt_mixed_causal_owner *,
    struct pt_mixed_readers_output *,uint64_t session,uint64_t generation);
int pt_mixed_causal_factory_stop_first_current(struct pt_mixed_causal_owner *,
    uint64_t predecessor);
/* Distinct finite TRIGGER -> CONTROL -> STOP factory checks. Only the actual
 * once-installed composite port, its matching derived CONTROL context/extent/
 * callback and the original owner registration qualify. Empty custody is
 * required before construction; subsequent predicates require the actual
 * completed TRIGGER or actual completed CONTROL plus clean genuine C1 detach.
 * They refuse once the corresponding next command has transferred to queue.
 * These task-only booleans return no key, queue, READY, fire or quiet authority;
 * typed admission must still repeat its genuine ACTIVE getters and all guards.
 * No callback, clock, source acquisition or publication runs here. Reentry
 * faults the owner through the existing task exclusion and returns zero. */
int pt_mixed_causal_factory_control_stop_original_empty(struct pt_mixed_causal_owner *,
    struct pt_mixed_readers_output *,uint64_t session,uint64_t generation);
int pt_mixed_causal_factory_control_stop_first_current(struct pt_mixed_causal_owner *,
    uint64_t predecessor);
int pt_mixed_causal_factory_control_stop_control_current(struct pt_mixed_causal_owner *,
    uint64_t predecessor);
struct pt_mixed_causal_factory_bind_result {int raw;unsigned called,current;};
/* One exact original callback under genuine owner task exclusion. Its supplied
 * context/extent MUST be the originally installed port context/extent. Copies
 * the core registration, guards the complete local object before initialization,
 * preserves raw reply and separately classifies identity/fault/reentry. No
 * shutdown, binding retry, queue exposure, source acquisition or READY proof.
 * Facade calls it once before factory and enforces actual called/current/raw1;
 * the helper itself does not latch callback readiness for a direct factory. */
struct pt_mixed_causal_factory_bind_result pt_mixed_causal_factory_bind_original(
    struct pt_mixed_causal_owner *,
    int (*)(void *,const struct pt_mixed_causal_registration *),void *,size_t);
/* Same genuine task services as the default queue, wrapped in causal owner
 * exclusion. Readiness never returns a key/prediction. Retirement preserves
 * actual consumed R under stale cleanup; it proves neither C nor source quiet. */
enum pt_mixed_readers_result pt_mixed_causal_factory_reader_readiness(
    struct pt_mixed_causal_owner *,uint64_t,unsigned);
struct pt_mixed_reader_retirement pt_mixed_causal_factory_retire_original(
    struct pt_mixed_causal_owner *,uint64_t,unsigned,unsigned);
#endif
