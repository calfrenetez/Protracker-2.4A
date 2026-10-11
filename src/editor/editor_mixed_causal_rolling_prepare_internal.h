#ifndef PT_EDITOR_MIXED_CAUSAL_ROLLING_PREPARE_INTERNAL_H
#define PT_EDITOR_MIXED_CAUSAL_ROLLING_PREPARE_INTERNAL_H
#include "editor_mixed_causal_lineage_control16_prepare_internal.h"
#include "sampler_mixed_causal_rolling_internal.h"
struct pt_editor_mixed_causal_rolling_inputs {
    struct pt_editor_mixed_causal_lineage_inputs original;
    struct pt_mixed_causal_rolling_port rolling;
};
/* Numeric row identities remain opaque after actual lower NULL consumption. */
struct pt_editor_mixed_causal_rolling_row {
    struct pt_editor_mixed_command_ref ref;
    uint64_t ticket,serial;
    unsigned detached,closed;
};
struct pt_editor_mixed_causal_rolling_state {
    struct pt_editor_mixed_causal_rolling_state *self;
    struct pt_editor_mixed_causal_lineage_prepare *preparation;
    const struct pt_editor_mixed_causal_rolling_inputs *inputs;
    struct pt_editor_mixed_causal_rolling_inputs saved;
    struct pt_sampler_mixed_causal_rolling_binding factory;
    struct pt_sampler_storage_span wrapper;
    unsigned joint;
    struct pt_editor_mixed_causal_rolling_row row[2];
};
struct pt_editor_mixed_causal_rolling_prepare {
    struct pt_editor_mixed_causal_lineage_prepare original;
    struct pt_editor_mixed_causal_rolling_state rolling;
};
/* original.lineage is zero and cannot grant old finite-three authority. */
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_rolling_prepare_begin(
    struct pt_editor_mixed_causal_rolling_prepare *,const struct pt_editor_mixed_causal_rolling_inputs *);
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_rolling_control16_prepare_batch_begin(
    struct pt_editor_mixed_causal_lineage_prepare *,const struct pt_editor_mixed_causal_lineage_control16_batch *,
    struct pt_editor_mixed_command_ref *);
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_rolling_stop_prepare_batch_begin(
    struct pt_editor_mixed_causal_lineage_prepare *,const struct pt_editor_mixed_causal_lineage_stop_batch *,
    struct pt_editor_mixed_command_ref *);
#endif
