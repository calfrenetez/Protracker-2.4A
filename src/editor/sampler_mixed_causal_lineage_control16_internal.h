#ifndef PT_SAMPLER_MIXED_CAUSAL_LINEAGE_CONTROL16_INTERNAL_H
#define PT_SAMPLER_MIXED_CAUSAL_LINEAGE_CONTROL16_INTERNAL_H
#include "sampler_mixed_causal_lineage_internal.h"
/* PRIVATE additive final-register-width CONTROL input. Original factory-issued
 * R handles alone authorize the existing master/cache/route/slot/key. Numeric
 * left/right are copied uint16 levels, not gains; no scale/clip/caller key.
 * Paula still requires rate/left/right0; AmiGUS requires period/volume0.
 * The same genuine composite binding, completed first and ACTIVE getters,
 * original full-span guards and consumed stage/allocator/enqueue limits apply.
 * One C and zero new R/pins/leases/cache/conversion/upload; existing independent
 * C/R/SOURCE quiet and lower-OK transfer rules remain. Old input/layout/API is
 * unchanged. No new mode flag, READY authority or binding capability is exposed.
 * SOURCE prototype only; no host/native/physical acceptance is asserted. */
struct pt_sampler_mixed_causal_lineage_control16_action {
    struct pt_sampler_mixed_reader_handle reader;
    uint16_t period;uint8_t volume;
    uint32_t rate;uint16_t left,right;
};
struct pt_sampler_mixed_causal_lineage_control16_batch {
    uint64_t frame;unsigned count;
    struct pt_sampler_mixed_causal_lineage_control16_action action[PT_SAMPLER_MIXED_ACTIONS];
};
enum pt_sampler_mixed_result pt_sampler_mixed_causal_lineage_control16_begin(
    struct pt_sampler_mixed_pool *,uint32_t,
    const struct pt_sampler_mixed_causal_lineage_control16_batch *,
    struct pt_sampler_mixed_command_handle *);
#endif
