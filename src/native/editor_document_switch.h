#ifndef PT_NATIVE_EDITOR_DOCUMENT_SWITCH_H
#define PT_NATIVE_EDITOR_DOCUMENT_SWITCH_H
#include "../core/document.h"
#include "../editor/editor.h"

/* Preparation/cancellation only: no stop, disposal, publication or GUI effects.
 * Serialized ordinary caller owns stationary, zero, aligned control storage and
 * the ORIGINAL document/editor/complete allocator context. Capacities describe
 * complete truthful allocations, not just prefixes. Fresh allocator results must
 * be disjoint from every living allocation, including opaque editor resources;
 * the known-span checks below are not a heap census or pointer authentication.
 *
 * The full original editor/document bytes remain unchanged until cancel ends.
 * No editing, navigation, hook changes or direct child API use is allowed while
 * this preparation is active. Comparison snapshots are NOT additional owners.
 * Load bytes must stay stable for the call, but are not retained after return.
 * The module calls the genuine document factory on its detached candidate; it
 * neither decodes data itself nor invokes an old editor/change hook.
 *
 * PREPARED retains candidate + original domain. Publication always returns
 * STOP_UNQUALIFIED in this first increment: neither a bool/callback nor a copied
 * header can supply missing native stop/adoption authority. Cancel genuinely
 * checked-releases the candidate once. HELD retains every original obligation
 * and permits no cancel/reset/retry/free; the native caller must keep its Task
 * and whole domain alive. This is not native HOLD/device quiet qualification.
 * Ended state/reset use local control bytes only and never traverse old inputs.
 */
struct pt_native_document_switch_inputs {
    struct pt_document *document;size_t document_capacity;
    struct pt_editor *editor;size_t editor_capacity;
    void *allocator_context;size_t allocator_context_capacity;
};
enum pt_native_document_switch_result {
    PT_NATIVE_DOCUMENT_SWITCH_EMPTY,
    PT_NATIVE_DOCUMENT_SWITCH_INVALID,
    PT_NATIVE_DOCUMENT_SWITCH_BUSY,
    PT_NATIVE_DOCUMENT_SWITCH_PREPARED,
    PT_NATIVE_DOCUMENT_SWITCH_FAILED,
    PT_NATIVE_DOCUMENT_SWITCH_CANCELED,
    PT_NATIVE_DOCUMENT_SWITCH_STOP_UNQUALIFIED,
    PT_NATIVE_DOCUMENT_SWITCH_HELD
};
struct pt_native_document_switch {
    struct pt_native_document_switch *self;
    size_t capacity;
    struct pt_native_document_switch_inputs inputs;
    struct pt_document candidate,candidate_snapshot,original_document;
    struct pt_editor original_editor; /* Comparison bytes only; never dispose. */
    enum pt_project_result preparation_result;
    enum pt_native_document_switch_result result;
    unsigned busy;
};
enum pt_native_document_switch_result pt_native_document_switch_new(
    struct pt_native_document_switch *,size_t,
    const struct pt_native_document_switch_inputs *,unsigned,size_t);
enum pt_native_document_switch_result pt_native_document_switch_load(
    struct pt_native_document_switch *,size_t,
    const struct pt_native_document_switch_inputs *,const uint8_t *,size_t,size_t);
enum pt_native_document_switch_result pt_native_document_switch_state(
    struct pt_native_document_switch *);
enum pt_native_document_switch_result pt_native_document_switch_admit_publication(
    struct pt_native_document_switch *);
enum pt_native_document_switch_result pt_native_document_switch_cancel(
    struct pt_native_document_switch *);
/* Only FAILED/CANCELED with a genuinely empty candidate may reset. Reset clears
 * sizeof(control) only; tail bytes are preserved and all old control aliases end.
 * No input, source or allocator context is dereferenced by ended state/reset. */
int pt_native_document_switch_reset(struct pt_native_document_switch *);
#endif
