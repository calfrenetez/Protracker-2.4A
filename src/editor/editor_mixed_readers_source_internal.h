#ifndef PT_EDITOR_MIXED_READERS_SOURCE_INTERNAL_H
#define PT_EDITOR_MIXED_READERS_SOURCE_INTERNAL_H
#include "editor_mixed_readers_prepare.h"

/* Private owner-thread seam, not a public producer/validation certificate.
 * The constructor installs the controller's existing static editor hook before
 * any external master establishment callback. Zero/noncopyable controller and
 * ORIGINAL separately mutable publisher, immutable input descriptor, complete
 * context aggregate and every full declared extent survive final positive
 * prepare_close/mixed_stop. No supplied callback, READY flag or quiet assertion.
 * contexts contains the whole controller plus ordinary callback contexts; it
 * excludes the original publisher and declared mutable result/output buffers.
 * preparation is its exact original future input, inside contexts or separate;
 * its values are admitted only by activate. Immutable spans guard outputs and
 * allocation returns. Mutable spans guard allocation returns only: original
 * request/ref/result publishers can remain writable by their genuine APIs.
 * All nonempty declared extents are pairwise apart and apart from contexts,
 * preparation, this input, publisher and all full project/sampler capacities.
 * Unused span entries are semantic zero. Fourteen is a total caller-span bound,
 * not a permission to truncate an outer parent or omit its unused capacity. */
struct pt_editor_mixed_source_inputs {
    struct pt_editor_mixed *binding;
    struct pt_sampler_storage_span contexts;
    const struct pt_editor_mixed_readers_prepare_inputs *preparation;
    /* Exact full roles, each matching one complete immutable span. Future
     * preparation fields may be unset here; activation later must match both
     * workspace roles exactly and keep full backend/reservation objects inside
     * the fixed whole backend parent. No generic immutable reuse permission. */
    struct pt_sampler_storage_span activation_workspace,factory_workspace,backend_parent;
    unsigned immutable_count,mutable_count;
    struct pt_sampler_storage_span immutable[PT_EDITOR_MIXED_SOURCE_SPANS];
    struct pt_sampler_storage_span mutable[PT_EDITOR_MIXED_SOURCE_SPANS];
};
struct pt_editor_mixed_source_borrow {
    struct pt_editor_mixed_readers_prepare *address;
    uint64_t serial;
};
enum pt_editor_mixed_readers_result pt_editor_mixed_source_begin(
    struct pt_editor_mixed_readers_prepare *,const struct pt_editor_mixed_source_inputs *,
    struct pt_editor_mixed_source_borrow *original);
/* Require actual producer establishment/cancellation and independent current
 * pins before activate; no argument claims them and this seam does not certify
 * them. Genuine factory validation follows. No callback/allocation here.
 * Retain original numeric guards and append actual current storage before the
 * one legitimate snapshot refresh. Failure retains early hook/borrow adoption. */
enum pt_editor_mixed_readers_result pt_editor_mixed_source_activate(
    struct pt_editor_mixed_readers_prepare *,const struct pt_editor_mixed_source_borrow *original,
    const struct pt_editor_mixed_readers_prepare_inputs *original_inputs);
/* Numeric read-only child-consumption query, never work/READY authority. */
int pt_editor_mixed_source_children_closed(struct pt_editor_mixed_readers_prepare *,
    const struct pt_editor_mixed_source_borrow *original);
/* Separate exclusion around actual external source child calls. Enter after
 * cancellation/fault is cleanup-only by protocol, not work permission. Retain
 * actual child outputs/NULL consumption before leave. Invalid/copied entry
 * never clears a live latch. Leave permits exact original cleanup after stale
 * fixed tags and never traverses former source arrays. Controller public calls
 * retain their own busy guard and must not be wrapped by source_enter. */
int pt_editor_mixed_source_enter(const struct pt_editor_mixed_source_borrow *original);
int pt_editor_mixed_source_leave(const struct pt_editor_mixed_source_borrow *original);
/* Close external sequence/Q/audit/establishment owners, independently consume
 * controller children/source quiet and unpin masters ONCE while borrow vetoes
 * mutation, then close this exact original publisher. Actual consumed zero is
 * terminal even when a child returned failure. Borrow release keeps hook and
 * storage alive until separate final positive prepare_close/mixed_stop. */
int pt_editor_mixed_source_borrow_close(struct pt_editor_mixed_source_borrow *original);
#endif
