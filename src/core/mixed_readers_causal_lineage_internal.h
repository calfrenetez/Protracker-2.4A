#ifndef PT_MIXED_READERS_CAUSAL_LINEAGE_INTERNAL_H
#define PT_MIXED_READERS_CAUSAL_LINEAGE_INTERNAL_H
#include "mixed_readers_causal_control_internal.h"
#include "mixed_readers_causal_stop_internal.h"

/* PRIVATE finite after-first TRIGGER -> CONTROL -> STOP mode. Two live C slots
 * remain: C1 must actually detach through the genuine queue before C3 admission.
 * Each frame/window remains original. This is no native aperture/timer proof. */
#define PT_MIXED_CAUSAL_CONTROL_STOP_VERSION 1U
#define PT_MIXED_CAUSAL_CONTROL_STOP_REQUIRED 3U
struct pt_mixed_causal_completed_value {
    struct pt_mixed_causal_command_identity identity;
    uint64_t frame,first_tick,last_tick,observed,issued;
    struct pt_mixed_causal_actual post;
};
struct pt_mixed_causal_control_stop_publication {
    struct pt_mixed_causal_completed_value root,control;
    struct pt_mixed_causal_command_identity stop;
    uint64_t serial;
    struct pt_mixed_causal_packet packet;
};
/* Distinct empty-owner declaration, complete original guarded context only.
 * The port independently compares its own actual root AND CONTROL completion
 * and complete all20 registry, copies accepted values, and retains no pointer.
 * Disposed event/binding pointers are opaque original-session identities only.
 * C2/C3 never retrigger, acquire, convert, upload or replace a reader/cache/master.
 * Existing command/reader/SOURCE quiet callbacks cover all expected-only keys.
 * 1 accepted,0 unchanged/no new reference,-1 uncertain; exclusion/mutation fault
 * retains possible ownership regardless of the callback's raw reply. */
struct pt_mixed_causal_control_stop_port {
    void *context;size_t context_bytes;unsigned version,flags;
    int (*publish_control)(void *,struct pt_mixed_causal_owner *,
        const struct pt_mixed_causal_control_publication *);
    int (*publish_stop_after_control)(void *,struct pt_mixed_causal_owner *,
        const struct pt_mixed_causal_control_stop_publication *);
};
enum pt_mixed_readers_result pt_mixed_causal_control_stop_bind(
    struct pt_mixed_causal_owner *,const struct pt_mixed_causal_control_stop_port *);
/* Existing control_enqueue/publish provide exactly the second CONTROL here.
 * Ordinary CONTROL-only/STOP-only/default owners do not gain this authority.
 * In this request only, predecessor means the exact second CONTROL ticket.
 * Actual CONTROL completion, internal exact C1 queue service OK(cancel0), and
 * every genuine original ACTIVE key are independently required. No caller keys.
 * Successful genuine enqueue alone transfers C3 even on an outer callback fault.
 * Selected R close to admission at publication, then DRAINING at exact commit;
 * independent actual C/R/SOURCE proofs, not completion/counts, govern release. */
enum pt_mixed_readers_result pt_mixed_causal_stop_after_control_enqueue(
    struct pt_mixed_causal_owner *,const struct pt_mixed_causal_stop_request *,uint64_t *);
enum pt_mixed_readers_result pt_mixed_causal_stop_after_control_publish(
    struct pt_mixed_causal_owner *,uint64_t);
#endif
