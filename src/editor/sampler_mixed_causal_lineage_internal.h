#ifndef PT_SAMPLER_MIXED_CAUSAL_LINEAGE_INTERNAL_H
#define PT_SAMPLER_MIXED_CAUSAL_LINEAGE_INTERNAL_H
#include "sampler_mixed_causal_internal.h"
/* PRIVATE opt-in finite TRIGGER -> CONTROL -> STOP factory mode. The entire
 * original aggregate is zero, retained, noncopyable and covered by original
 * complete config contexts. The genuine installed composite capability is
 * checked at bind and open. A default, CONTROL-only or STOP-only owner cannot
 * acquire this mode. One constructor attempt; never reset/edit the binding.
 * This is ordinary HOST preparation, not native activation/device-stop proof. */
struct pt_sampler_mixed_causal_lineage_binding {
    struct pt_sampler_mixed_causal_binding original;
    struct pt_sampler_mixed_causal_lineage_binding *self;
};
int pt_sampler_mixed_causal_lineage_bind(struct pt_sampler_mixed_causal_lineage_binding *,
    struct pt_mixed_causal_owner *,struct pt_mixed_readers_output *,
    uint64_t,uint64_t,struct pt_mixed_readers_span);
enum pt_sampler_mixed_result pt_sampler_mixed_causal_lineage_open(
    const struct pt_sampler_mixed_config *,uint32_t,void *,size_t,
    struct pt_sampler_mixed_causal_lineage_binding *,struct pt_sampler_mixed_pool **);
/* Only original factory-issued R handles and numeric CONTROL values. Route,
 * slot, source and key are derived internally. Paula requires rate/left/right0;
 * AmiGUS requires period/volume0. Every unused declared field is zero. */
struct pt_sampler_mixed_causal_lineage_control_action {
    struct pt_sampler_mixed_reader_handle reader;
    uint16_t period;uint8_t volume;
    uint32_t rate;uint8_t left,right;
};
struct pt_sampler_mixed_causal_lineage_control_batch {
    uint64_t frame;unsigned count;
    struct pt_sampler_mixed_causal_lineage_control_action action[PT_SAMPLER_MIXED_ACTIONS];
};
struct pt_sampler_mixed_causal_lineage_stop_batch {
    uint64_t frame;unsigned count;
    struct pt_sampler_mixed_reader_handle reader[PT_SAMPLER_MIXED_ACTIONS];
};
/* First TRIGGER uses the original begin/quantized-begin. Each later stage first
 * verifies actual completed predecessor and genuine ACTIVE original getters,
 * then consumes its one allocator attempt in the genuine pool. Refusal before
 * that boundary is unchanged; consumed construction and lower enqueue attempts
 * cannot retry or admit a fourth.
 * CONTROL/STOP allocate exactly one C and zero R/pins/leases/cache/uploads.
 * Two live C/32 R limits remain. C3 additionally requires actual clean C1 queue
 * service and actual RETIRED first-handle NULL consumption; no C1 pointer is
 * retained. Original absolute frames/windows and independent C/R/SOURCE proofs
 * govern admission/publication/retirement. Lower enqueue OK remains transfer
 * authority even when a later outer callback detects a fault. */
enum pt_sampler_mixed_result pt_sampler_mixed_causal_lineage_control_begin(
    struct pt_sampler_mixed_pool *,uint32_t,
    const struct pt_sampler_mixed_causal_lineage_control_batch *,
    struct pt_sampler_mixed_command_handle *);
enum pt_sampler_mixed_result pt_sampler_mixed_causal_lineage_stop_begin(
    struct pt_sampler_mixed_pool *,uint32_t,
    const struct pt_sampler_mixed_causal_lineage_stop_batch *,
    struct pt_sampler_mixed_command_handle *);
#endif
