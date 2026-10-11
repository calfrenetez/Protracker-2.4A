#ifndef PT_MIXED_READERS_CAUSAL_ROLLING_INTERNAL_H
#define PT_MIXED_READERS_CAUSAL_ROLLING_INTERNAL_H
#include "mixed_readers_causal_lineage_internal.h"
#include "mixed_readers_causal_control16_internal.h"

/* PRIVATE SOURCE proposal. Implementation is authored, execution is disabled.
 * A distinct private capability; no CONTROL-only or finite-three forwarding.
 * The first producer profile is exactly TRIGGER16, CONTROL16, CONTROL16,
 * STOP16. This declaration does not enable PLAY or arbitrary song shapes.
 * Root/latest values come only from actual successful core and port commits.
 * Historical owner/event/binding addresses are opaque identities, never read.
 */
#define PT_MIXED_CAUSAL_ROLLING_VERSION 1U
#define PT_MIXED_CAUSAL_ROLLING_REQUIRED 7U
struct pt_mixed_causal_rolling_publication {
    struct pt_mixed_causal_completed_value root,predecessor;
    struct pt_mixed_causal_command_identity next;
    uint64_t serial;
    enum pt_mixed_readers_kind kind;
    struct pt_mixed_causal_packet packet;
    /* Exact old row identity comes from actual core completion, independent
     * command quiet and genuine service OK(cancel0), never caller row counts.
     * All zero for the first successor. Historical pointers stay opaque. */
    struct pt_mixed_causal_command_identity reuse;
    uint64_t reuse_serial;
    unsigned reuse_required;
};
struct pt_mixed_causal_rolling_port {
    void *context;
    size_t context_bytes;
    unsigned version,flags;
    /* Serial1 names the actual root completion; later serials name the actual
     * latest CONTROL completion. Independently authenticate complete all20
     * current keys, original absolute frame/window and original registration.
     * A row overwrite additionally requires positive independent ticket quiet.
     * Copy accepted values; retain no caller pointer. Raw1 accepted,0 no new
     * reference,-1 uncertain; mutation/reentry stays fatal regardless of raw.
     */
    int (*publish_next)(void *,struct pt_mixed_causal_owner *,
        const struct pt_mixed_causal_rolling_publication *);
};
enum pt_mixed_readers_result pt_mixed_causal_rolling_bind(
    struct pt_mixed_causal_owner *,const struct pt_mixed_causal_rolling_port *);
/* Distinct typed entries. Existing request structs supply numeric fields and
 * an exact predecessor ticket, never keys/adoption/quiet/disposal authority.
 * Root readers remain the genuine original factory handles through STOP and
 * independent reader/source quiet. Actual lower enqueue OK alone transfers.
 */
enum pt_mixed_readers_result pt_mixed_causal_rolling_control16_enqueue(
    struct pt_mixed_causal_owner *,const struct pt_mixed_causal_control16_request *,uint64_t *);
enum pt_mixed_readers_result pt_mixed_causal_rolling_stop_enqueue(
    struct pt_mixed_causal_owner *,const struct pt_mixed_causal_stop_request *,uint64_t *);
enum pt_mixed_readers_result pt_mixed_causal_rolling_publish(
    struct pt_mixed_causal_owner *,uint64_t);
/* Typed publish accepts the original root TRIGGER ticket as well. Every arm
 * epoch is once-only, including clean0. Refusal preserves actual queue custody.
 */
/* Task-only genuine-current query for the shared factory/controller. This
 * invokes actual same-owner queue ACTIVE getters; true is no copied capability
 * and never grants fire/quiet/disposal/ownership authority. The implementation
 * must check the distinct installed rolling scope, original full registry and
 * latest actual completion. A free row alone cannot authorize admission.
 */
int pt_mixed_causal_factory_rolling_current(
    struct pt_mixed_causal_owner *,uint64_t predecessor);
int pt_mixed_causal_factory_rolling_original_empty(
    struct pt_mixed_causal_owner *,struct pt_mixed_readers_output *,
    uint64_t session,uint64_t generation);
/* Owner-generated next epoch: root0, then CONTROL1/CONTROL2/STOP3 in the first
 * producer profile. The core permits further typed CONTROL epochs until STOP;
 * the producer independently bounds its four-shape profile. reuse is zero at
 * first successor, otherwise the opposite original C ticket whose independent
 * quiet and actual service OK(cancel0) consumed the genuine queue command.
 * The factory MUST additionally prove that exact original C's NULL disposal.
 * Numeric values grant no adoption, quiet, ownership or fire authority. */
struct pt_mixed_causal_rolling_basis {
    uint64_t root,predecessor,serial,reuse;
    unsigned reuse_required;
};
int pt_mixed_causal_factory_rolling_basis(
    struct pt_mixed_causal_owner *,struct pt_mixed_causal_rolling_basis *);
#endif
