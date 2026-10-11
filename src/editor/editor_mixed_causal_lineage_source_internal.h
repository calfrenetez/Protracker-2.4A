#ifndef PT_EDITOR_MIXED_CAUSAL_LINEAGE_SOURCE_INTERNAL_H
#define PT_EDITOR_MIXED_CAUSAL_LINEAGE_SOURCE_INTERNAL_H
#include "editor_mixed_causal_source_internal.h"
#include "editor_mixed_causal_lineage_prepare_internal.h"

/* PRIVATE joint early SOURCE + original finite lineage scope. This is not a
 * producer, source-readiness certificate or new backend/fire authority. Keep
 * this ENTIRE zero/noncopyable wrapper inside the complete ordinary contexts
 * parent. The original separately mutable publisher and full source descriptor
 * stay outside that parent; complete immutable/mutable capacities stay guarded.
 * preparation is the exact future complete composite input, not a narrowed
 * original prefix: original.preparation must point at its original member.
 * Its body may be established before the sole activate; afterwards immutable.
 * SOURCE descriptor itself remains immutable from begin through final close.
 */
struct pt_editor_mixed_causal_lineage_source_inputs {
    struct pt_editor_mixed_causal_source_inputs original;
    const struct pt_editor_mixed_causal_lineage_inputs *preparation;
};
struct pt_editor_mixed_causal_lineage_source_prepare {
    struct pt_editor_mixed_causal_lineage_prepare original;
    struct pt_editor_mixed_causal_lineage_source_prepare *self;
    const struct pt_editor_mixed_causal_lineage_source_inputs *inputs;
    struct pt_editor_mixed_causal_lineage_source_inputs saved;
};
typedef char pt_lineage_source_original_prefix[
    offsetof(struct pt_editor_mixed_causal_lineage_source_prepare,original)==0?1:-1];
typedef char pt_lineage_source_descriptor_prefix[
    offsetof(struct pt_editor_mixed_causal_lineage_source_inputs,original)==0?1:-1];

/* Admission/editing veto only: no callback, master establishment, persistent
 * pin, workspace initialization, cache, registration or factory acquisition.
 * The caller performs genuine SOURCE-protected establishment/current pins.
 * Full incoming wrapper/descriptor/composite/publisher/source/declared extents
 * are checked before an owned write or new original publisher is issued.
 */
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_lineage_source_begin(
    struct pt_editor_mixed_causal_lineage_source_prepare *,
    const struct pt_editor_mixed_causal_lineage_source_inputs *,
    struct pt_editor_mixed_causal_source_borrow *original);
/* Exact original publisher and future composite input only. Admit whole local
 * snapshot before its first write. One legitimate current-storage refresh;
 * actual composite is installed before registration/factory on later advance.
 * No argument says READY, ACTIVE, quiet or established. Copied/bypass activation
 * refuses; failure retains original SOURCE custody for independent cleanup.
 * Reuse original SOURCE enter/leave/observe/retirement/borrow-close and original
 * controller advance/enqueue/publish/service/cancel/close, naming original.original.
 * Both SOURCE and complete composite identities/current inputs are enforced.
 * Ordinary SOURCE mode and standalone lineage retain their existing paths.
 */
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_lineage_source_activate(
    struct pt_editor_mixed_causal_lineage_source_prepare *,
    const struct pt_editor_mixed_causal_source_borrow *original,
    const struct pt_editor_mixed_causal_lineage_inputs *original_inputs);
#endif
