/* Private ordinary-RAM IRQ contract seam. Native binding is INHIBITED.
 * HOST model tags are explicit fixed-width identities, never truncated pointers.
 * No timer/vector acquisition, acknowledgement, cancellation or release is here.
 * Those independent obligations remain the genuine adapter's responsibility.
 */
#ifndef PT_PRIVATE_NATIVE_MIXED_CAUSAL_ROLLING_IRQ_BRIDGE_H
#define PT_PRIVATE_NATIVE_MIXED_CAUSAL_ROLLING_IRQ_BRIDGE_H
#include "native_mixed_causal_ram_port.h"
#include "native_mixed_causal_rolling_irq_layout.h"
#if defined(__m68k__) && defined(PT_PRIVATE_ROLLING_IRQ_HOST_MODEL)
#error "HOST IRQ model cannot admit a native provider"
#endif
#define PT_PRIVATE_ROLLING_IRQ_TASK 1U
#define PT_PRIVATE_ROLLING_IRQ_IRQ 2U
#define PT_PRIVATE_ROLLING_IRQ_ARMED 1U
#define PT_PRIVATE_ROLLING_IRQ_EARLY 2U
#define PT_PRIVATE_ROLLING_IRQ_COMPLETED 3U
#define PT_PRIVATE_ROLLING_IRQ_UNCERTAIN 4U
struct pt_private_rolling_irq_ops {
    void *context;size_t context_bytes;
    /* Trusted whole-entry primitive, exact prior-state token,0 refuses with
     * no change. Restore is synchronous and cannot fail. A restored TASK
     * exclusion may deliver a deferred IRQ before it returns; all storage
     * must still live. IRQ uses no task scheduler or allocation callback. */
    uint64_t (*exclude)(void *,unsigned);
    void (*restore)(void *,unsigned,uint64_t);
    int (*clock)(void *,uint64_t *,uint32_t *);
};
struct pt_private_rolling_irq_tags {
    uint32_t bridge,self,owner,queue; /* HOST-only declared address model. */
};
struct pt_private_rolling_irq_record {
    struct pt_mixed_causal_command_identity identity;
    uint64_t first,last,frame,serial;
    uint32_t frequency;
    unsigned row,known,armed,completed,uncertain;
    int arm_raw,dispatch_raw,ledger_raw;
};
struct pt_private_rolling_irq_bridge {
    struct pt_private_rolling_irq_bridge *self;
    struct pt_private_native_rolling_irq32 *envelope;
    struct pt_private_rolling_irq_ops ops;
    struct pt_private_rolling_irq_tags tags;
    struct pt_private_mixed_causal_ram_port *port;
    struct pt_mixed_causal_registration registration;
    struct pt_private_rolling_irq_record original[4];
    uint64_t exclusion_token;
    unsigned initialized,bound,host_model,native_inhibited;
    unsigned scope,busy,ops_busy,dispatching,failed,faults;
    unsigned staged,current,irq_entries,dispatches,restores;
};
/* Fresh complete ZERO and fixed disjoint spans, noncopyable, once-only bind.
 * This is explicitly a HOST constructor, not a native-provider constructor.
 * Native pointer admission, A4 convention, compiled complete callgraph,
 * residency, genuine CIA/task exclusion, WCET/aperture and aggregate stack
 * remain absent. The assembly's C entry refuses before any payload read.
 * All bridge/envelope/port/context/code storage lives through genuine source
 * quiet AND the full outer leave/restore, even after command consumption.
 */
#ifdef PT_PRIVATE_ROLLING_IRQ_HOST_MODEL
int pt_private_rolling_irq_host_init(struct pt_private_rolling_irq_bridge *,
    struct pt_private_native_rolling_irq32 *,const struct pt_private_rolling_irq_ops *,
    const struct pt_private_rolling_irq_tags *);
int pt_private_rolling_irq_host_bind(struct pt_private_rolling_irq_bridge *,
    struct pt_private_mixed_causal_ram_port *,const struct pt_mixed_causal_registration *);
int pt_private_rolling_irq_task_enter(struct pt_private_rolling_irq_bridge *);
int pt_private_rolling_irq_task_leave(struct pt_private_rolling_irq_bridge *);
/* Genuine RAM arm callback only: after actual pre-clock and complete RAM/core
 * admission, before independent adapter arm. No caller-issued packet/identity.
 * Records all four originals. Actual row reuse quiet+retirement is still checked
 * by genuine core/factory/RAM before this callback, not inferred from serial. */
int pt_private_rolling_irq_stage(struct pt_private_rolling_irq_bridge *,uint64_t,uint64_t,uint32_t);
int pt_private_rolling_irq_arm_result(struct pt_private_rolling_irq_bridge *,uint64_t,int);
/* Clock adapter for genuine RAM/core pre/post calls. No fake completion. */
int pt_private_rolling_irq_clock(void *,uint64_t *,uint32_t *);
/* Explicit model entry; never uses envelope addresses as HOST pointers. It
 * A single unexposed fixed automatic HOST witness retains actual callback
 * reentry faults even if borrowed counters are rewritten. All witness anchors
 * retire immediately after each synchronous primitive call. This is not native
 * concurrency/ABI qualification. It calls genuine RAM dispatch, retaining raw
 * adoption separately from a guarded
 * failure. EARLY preserves this same original absolute window. No rearm occurs
 * here. Completion is never a command/reader/source quiet receipt. */
int pt_private_rolling_irq_host_entry(struct pt_private_rolling_irq_bridge *,
    struct pt_private_native_rolling_irq32 *);
#endif
#endif
