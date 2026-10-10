#ifndef PT_MIXED_READERS_CAUSAL_CONTROL_INTERNAL_H
#define PT_MIXED_READERS_CAUSAL_CONTROL_INTERNAL_H
#include "mixed_readers_causal.h"

/* PRIVATE after-first groundwork, not the prepublished two-trigger path.
 * Task preparation starts only AFTER actual first completion and genuine queue
 * ACTIVE observations. No native aperture, timer or exact live scheduling is
 * qualified. Caller supplies the original absolute frame; lateness never rebases.
 * Entire task/fire/input lifetime is externally serialized as for the owner.
 */
#define PT_MIXED_CAUSAL_CONTROL_VERSION 1U
#define PT_MIXED_CAUSAL_CONTROL_REQUIRED 3U
struct pt_mixed_causal_control_publication {
    struct pt_mixed_causal_command_identity predecessor,successor;
    uint64_t first_tick,last_tick,observed,issued,serial;
    struct pt_mixed_causal_actual first_post;
    struct pt_mixed_causal_packet packet;
};
/* A DISTINCT declared capability, never inferred from the old commit callback.
 * Context and its complete capacity must be the originally guarded port parent.
 * The callback must independently match its own genuine first completion and
 * complete actual all20 registry against the copied geometry-free tombstone,
 * then copy this CONTROL packet/identity before the original absolute deadline.
 * It must update existing reader geometry only, retaining the same cache/source
 * identities; no retrigger, acquisition, upload, replacement or new reader.
 * 1 accepted,0 unchanged/no retained reference,-1 uncertain; mutation or a
 * detected task/fire exclusion fault turns raw0 into retained uncertainty.
 * Refusal-only invalid recursive entries need not fault. Independent C/R/source quiet
 * callbacks still cover this callback and every expected-only dependency.
 * Pointer fields, including a disposed/recycled first event/binding, are opaque
 * identities distinguished by original session/ticket; never dereference them.
 */
struct pt_mixed_causal_control_port {
    void *context;size_t context_bytes;unsigned version,flags;
    int (*publish_control)(void *,struct pt_mixed_causal_owner *,
        const struct pt_mixed_causal_control_publication *);
};
/* Control-only numeric fields. All unused/opposite-route fields must be zero.
 * There is no caller reader key, domain, source/cache pointer or sample geometry.
 */
struct pt_mixed_causal_control_action {
    unsigned route,slot;
    uint16_t period;uint8_t volume;
    uint32_t rate;uint8_t left,right;
};
struct pt_mixed_causal_control_request {
    uint64_t predecessor,frame;
    unsigned count;
    struct pt_mixed_readers_control command;
    struct pt_mixed_causal_control_action action[PT_MIXED_READERS_ACTIONS];
};
/* Fresh empty-owner-only, once-only install before factory/first admission.
 * Copies the capability; no callback, clock, allocation or target action.
 * Default two-trigger APIs remain separate and unchanged. In this mode first
 * TRIGGER may complete alone; old successor/matcher entries refuse this owner.
 */
enum pt_mixed_readers_result pt_mixed_causal_control_bind(
    struct pt_mixed_causal_owner *,const struct pt_mixed_causal_control_port *);
/* Each selected target is derived from actual first post-state and independently
 * checked using the genuine queue ACTIVE key getter. Explicit prior C or R
 * service observations are required; adopted owner masks alone are insufficient.
 * At most one CONTROL command for this lifetime. Full request/ticket/holder and
 * transformed scratch spans are guarded before writes/callbacks; request and
 * command context remain immutable/alive throughout. Successful genuine enqueue
 * transfers C ownership even if a holder also faults the outer owner.
 */
enum pt_mixed_readers_result pt_mixed_causal_control_enqueue(
    struct pt_mixed_causal_owner *,const struct pt_mixed_causal_control_request *,uint64_t *);
enum pt_mixed_readers_result pt_mixed_causal_control_publish(
    struct pt_mixed_causal_owner *,uint64_t);
#endif
