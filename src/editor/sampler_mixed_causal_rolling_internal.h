#ifndef PT_SAMPLER_MIXED_CAUSAL_ROLLING_INTERNAL_H
#define PT_SAMPLER_MIXED_CAUSAL_ROLLING_INTERNAL_H
#include "sampler_mixed_causal_lineage_control16_internal.h"
#include "../core/mixed_readers_causal_rolling_internal.h"
/* Distinct constructor-issued capability. Whole binding stays in the original
 * protected contexts; no finite bind, caller mode or reconstructed epoch. */
struct pt_sampler_mixed_causal_rolling_binding {
    struct pt_sampler_mixed_causal_binding original;
    struct pt_sampler_mixed_causal_rolling_binding *self;
};
int pt_sampler_mixed_causal_rolling_bind(struct pt_sampler_mixed_causal_rolling_binding *,
    struct pt_mixed_causal_owner *,struct pt_mixed_readers_output *,
    uint64_t,uint64_t,struct pt_mixed_readers_span);
enum pt_sampler_mixed_result pt_sampler_mixed_causal_rolling_open(
    const struct pt_sampler_mixed_config *,uint32_t,void *,size_t,
    struct pt_sampler_mixed_causal_rolling_binding *,struct pt_sampler_mixed_pool **);
enum pt_sampler_mixed_result pt_sampler_mixed_causal_rolling_control16_begin(
    struct pt_sampler_mixed_pool *,uint32_t,
    const struct pt_sampler_mixed_causal_lineage_control16_batch *,
    struct pt_sampler_mixed_command_handle *);
enum pt_sampler_mixed_result pt_sampler_mixed_causal_rolling_stop_begin(
    struct pt_sampler_mixed_pool *,uint32_t,
    const struct pt_sampler_mixed_causal_lineage_stop_batch *,
    struct pt_sampler_mixed_command_handle *);
#endif
