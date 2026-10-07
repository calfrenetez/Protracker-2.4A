#ifndef PT_SAMPLER_MIXED_CAUSAL_INTERNAL_H
#define PT_SAMPLER_MIXED_CAUSAL_INTERNAL_H
#include "sampler_mixed_readers.h"
#include "../core/mixed_readers_causal.h"
/* Private one-time factory binding, never a queue key or READY certificate.
 * Fresh zero, ORIGINAL noncopyable slot lies in the complete retained ordinary
 * context aggregate. Bind immediately after genuine successful causal open and
 * exact queue borrow, before any factory constructor/cache/admission/publication.
 * Core verifies actual original owner/queue/session/generation and empty custody;
 * a supplied packet cannot teach this registration. Context aggregate includes
 * all original allocator/Chip/clock/task-port context capacities and survives
 * factory plus causal source closure. No source acquisition before binding.
 * One factory attempt only; failed attempt and consumed pool cannot reset/rebind.
 * Binding stays readable until owner is positively consumed. Do not edit fields.
 * This struct binding authenticates the genuine factory owner/queue only; it
 * neither invokes nor certifies a source callback. Direct private callers own
 * that original registration/source ordering separately. The new preparation
 * facade additionally enforces its mandatory bind_original callback raw1 under
 * genuine owner task exclusion before calling this factory constructor.
 */
struct pt_sampler_mixed_causal_binding {
    struct pt_sampler_mixed_causal_binding *self;
    struct pt_mixed_causal_owner *owner;
    struct pt_mixed_readers_output *queue;
    uint64_t session,generation;
    struct pt_mixed_readers_span contexts;
    unsigned phase;
};
int pt_sampler_mixed_causal_bind(struct pt_sampler_mixed_causal_binding *,
    struct pt_mixed_causal_owner *,struct pt_mixed_readers_output *,
    uint64_t session,uint64_t generation,struct pt_mixed_readers_span contexts);
/* Same genuine pool, validator, private input construction, full external
 * output guards and resource/pin/cache holders. Immutable opt-in mode dispatches
 * first/successor only to typed causal entries; default open remains raw path.
 * No CONTROL/STOP or third admission, rolling refill, prediction-as-ACTIVE,
 * timer/device wiring or whole-song claim. Actual OK alone transfers even on
 * outer callback fault. Caller inputs/workspace/output keep their full guards.
 */
enum pt_sampler_mixed_result pt_sampler_mixed_causal_open(
    const struct pt_sampler_mixed_config *,uint32_t,void *,size_t,
    struct pt_sampler_mixed_causal_binding *,struct pt_sampler_mixed_pool **);
#endif
