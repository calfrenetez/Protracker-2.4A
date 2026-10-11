#ifndef PT_EDITOR_MIXED_CAUSAL_ROLLING_SOURCE_INTERNAL_H
#define PT_EDITOR_MIXED_CAUSAL_ROLLING_SOURCE_INTERNAL_H
#include "editor_mixed_causal_lineage_source_internal.h"
#include "editor_mixed_causal_rolling_prepare_internal.h"
struct pt_editor_mixed_causal_rolling_source_inputs {
    struct pt_editor_mixed_causal_lineage_source_inputs original;
    const struct pt_editor_mixed_causal_rolling_inputs *preparation;
};
struct pt_editor_mixed_causal_rolling_source_prepare {
    struct pt_editor_mixed_causal_lineage_source_prepare original;
    struct pt_editor_mixed_causal_rolling_state rolling;
};
/* Full original.contexts covers this complete extended wrapper. The complete
 * descriptor and complete preparation are protected before the early hook.
 * SOURCE borrowing, promotion and terminal quiet share the original engine. */
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_rolling_source_begin(
    struct pt_editor_mixed_causal_rolling_source_prepare *,
    const struct pt_editor_mixed_causal_rolling_source_inputs *,
    struct pt_editor_mixed_causal_source_borrow *);
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_rolling_source_activate(
    struct pt_editor_mixed_causal_rolling_source_prepare *,
    const struct pt_editor_mixed_causal_source_borrow *,
    const struct pt_editor_mixed_causal_rolling_inputs *);
#endif
