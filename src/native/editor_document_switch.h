#ifndef PT_NATIVE_EDITOR_DOCUMENT_SWITCH_H
#define PT_NATIVE_EDITOR_DOCUMENT_SWITCH_H
#include "../core/document.h"
#include "../editor/editor.h"
#if defined(PT_NATIVE_DOCUMENT_SWITCH_COMMIT_HOST_ONLY)
#if PT_NATIVE_DOCUMENT_SWITCH_COMMIT_HOST_ONLY != 1 || defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA) || defined(__AROS__) || defined(__MORPHOS__)
#error "Document switch commit is an explicit private HOST composition, not native stop authority"
#endif
#include "../editor/editor_paula.h"
struct pt_native_document_switch_commit_inputs {
    struct pt_editor_paula *owner;size_t owner_capacity;
    void *release_context;size_t release_context_capacity;
};
#endif

/* Default preparation/cancellation only: no stop, disposal, publication or GUI
 * effects. The separately selected HOST commit extension is specified below.
 * Serialized ordinary caller owns stationary, zero, aligned control storage and
 * the ORIGINAL document/editor/complete allocator context. Capacities describe
 * complete truthful allocations, not just prefixes. Fresh allocator results must
 * be disjoint from every living allocation, including opaque editor resources;
 * the known-span checks below are not a heap census or pointer authentication.
 *
 * Preparation and cancellation keep the full original editor/document bytes
 * unchanged until cancel ends. The selected HOST commit has the separate
 * irreversible lifecycle specified below.
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
    PT_NATIVE_DOCUMENT_SWITCH_HELD,
    PT_NATIVE_DOCUMENT_SWITCH_COMMITTED
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
#if defined(PT_NATIVE_DOCUMENT_SWITCH_COMMIT_HOST_ONLY)
    struct pt_native_document_switch_commit_inputs commit;
    struct pt_allocator sampler_allocator,source_allocator,song_allocator;
    size_t sampler_budget,song_budget;
    unsigned commit_attempted,commit_phase;
#endif
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
/* Only FAILED/CANCELED (or HOST COMMITTED) with a genuinely empty candidate may reset. Reset clears
 * sizeof(control) only; tail bytes are preserved and all old control aliases end.
 * No input, source or allocator context is dereferenced by ended state/reset. */
int pt_native_document_switch_reset(struct pt_native_document_switch *);
#if defined(PT_NATIVE_DOCUMENT_SWITCH_COMMIT_HOST_ONLY)
/* Separate serialized opt-in commit for the genuine typed Paula owner ONLY.
 * No caller bool, stop callback, legacy native pt_paula or mixed/MIDI facade is
 * accepted. The exact original editor barrier must still belong to this owner.
 * All original controls, full allocations and opaque owner/output/allocator
 * contexts stay stationary and living through return (forever on HELD). The
 * supplied capacities describe complete real allocations, including tails;
 * opaque retained contexts must also be disjoint from control/document/editor,
 * candidate/storage and every living sampler allocation. There is no heap census.
 * No concurrent owner, MIDI sidecar, direct child call or mutation is supported.
 *
 * One genuine detach performs stop/readers/quiescence/release exactly once.
 * Refusal precedes editor/history disposal and retains both domains. After
 * confirmed detach, disposal is irreversible: later failure retains ALL
 * remaining obligations, not a playable old editor or a rollback guarantee.
 * Retired sampler/source/song and checked old document release precede the
 * sole candidate transfer. Init runs at the ORIGINAL editor address, restoring
 * allocator/budget policy on fresh sampler/source/song instances. No old live
 * editor/hook/control is copied or rebound. Init refusal retains the published
 * new document and whole context in HELD. COMMITTED transfers ownership to the
 * caller; ended state/reset never traverse the former original pointers.
 * This HOST seam cannot enable the native frontend. Default/native publication
 * remains STOP_UNQUALIFIED until a genuine checked native stop path exists. */
enum pt_native_document_switch_result pt_native_document_switch_commit_paula(
    struct pt_native_document_switch *,const struct pt_native_document_switch_commit_inputs *);
#endif
#endif
