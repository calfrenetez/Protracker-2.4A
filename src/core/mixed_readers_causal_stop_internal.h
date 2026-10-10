#ifndef PT_MIXED_READERS_CAUSAL_STOP_INTERNAL_H
#define PT_MIXED_READERS_CAUSAL_STOP_INTERNAL_H
#include "mixed_readers_causal.h"

/* PRIVATE HOST after-first finite TRIGGER -> STOP alternative successor.
 * Task preparation requires actual first completion and genuine queue ACTIVE
 * observations. No native aperture/timer or exact live scheduling is qualified.
 * Original absolute frames never rebase. External task/fire serialization and
 * complete immutable caller/context extents remain required as for the owner.
 */
#define PT_MIXED_CAUSAL_STOP_VERSION 1U
#define PT_MIXED_CAUSAL_STOP_REQUIRED 3U
struct pt_mixed_causal_stop_publication {
    struct pt_mixed_causal_command_identity predecessor,successor;
    uint64_t first_tick,last_tick,observed,issued,serial;
    struct pt_mixed_causal_actual first_post;
    struct pt_mixed_causal_packet packet;
};
/* DISTINCT once-only empty-owner capability, mutually exclusive with CONTROL.
 * Context/capacity must match the complete original guarded port parent.
 * Independently compare the port's actual first completion and all20 registry
 * to the geometry-free predecessor tombstone before copying this STOP packet.
 * Opaque disposed first pointers are identities only; never dereference them.
 * Accepted publication closes selected readers to further admission, but proves
 * no voice stop or reader quiet. Exact commit clears only selected actual slots;
 * original readers/cache/master leases remain independently held DRAINING.
 * No new representation, reader, acquisition, upload or retrigger is allowed.
 * 1 accepted,0 unchanged/no retained reference,-1 uncertain. Mutation or detected
 * task/fire exclusion fault turns raw0 into retained uncertainty. Refusal-only
 * invalid recursion need not fault. Independent C/R/source proofs include all
 * expected-only command dependencies, and never follow from STOP completion.
 */
struct pt_mixed_causal_stop_port {
    void *context;size_t context_bytes;unsigned version,flags;
    int (*publish_stop)(void *,struct pt_mixed_causal_owner *,
        const struct pt_mixed_causal_stop_publication *);
};
struct pt_mixed_causal_stop_action {unsigned route,slot;};
/* Unique original route/slot targets only. Unused actions must be zero.
 * No caller key/domain/cache/sample geometry. The new C holder is genuine.
 */
struct pt_mixed_causal_stop_request {
    uint64_t predecessor,frame;unsigned count;
    struct pt_mixed_readers_control command;
    struct pt_mixed_causal_stop_action action[PT_MIXED_READERS_ACTIONS];
};
enum pt_mixed_readers_result pt_mixed_causal_stop_bind(
    struct pt_mixed_causal_owner *,const struct pt_mixed_causal_stop_port *);
/* Actual queue ACTIVE getter per selected first reader is authority. Successful
 * genuine enqueue alone transfers C ownership, including callback-fault cases.
 * One STOP successor for this lifetime; no third lineage or sticky-state reset.
 * Local pt_mixed_causal_stop is shutdown/cancellation, not this musical STOP.
 */
enum pt_mixed_readers_result pt_mixed_causal_stop_enqueue(
    struct pt_mixed_causal_owner *,const struct pt_mixed_causal_stop_request *,uint64_t *);
enum pt_mixed_readers_result pt_mixed_causal_stop_publish(
    struct pt_mixed_causal_owner *,uint64_t);
#endif
