#ifndef PT_MIDI_SETUP_OWNER_H
#define PT_MIDI_SETUP_OWNER_H
/* A separate qualification-only partition. This does not admit a frontend,
 * document transition, interrupt callback or external MIDI/backend operation. */
#if defined(PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY)
#if PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY != 1 || !defined(PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY) || PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY != 1
#error "Native MIDI owner requires the exact paired TASK RAM probe options"
#endif
#if !defined(__m68k__) || !defined(__amigaos__) || defined(__AROS__) || defined(__MORPHOS__)
#error "Native MIDI owner TASK RAM probe requires the genuine classic target"
#endif
#if defined(PT_MIDI_SETUP_OWNER_HOST_ONLY) || defined(PT_MIDI_REQUESTER_OWNER_HOST_ONLY) || defined(PT_DOCUMENT_MIDI_SETUP_HOST_ONLY)
#error "Native MIDI owner TASK RAM probe excludes HOST/document composition"
#endif
#else
#if !defined(PT_MIDI_SETUP_OWNER_HOST_ONLY) || PT_MIDI_SETUP_OWNER_HOST_ONLY != 1
#error "Private MIDI setup owner requires explicit HOST-only selection"
#endif
#if defined(__amigaos__) || defined(__m68k__) || defined(__AROS__) || defined(__MORPHOS__)
#error "This private MIDI setup owner has no native admission"
#endif
#endif
#include "midi_setup.h"
struct pt_midi_setup_owner;
struct pt_midi_setup_owner_span {const void *data;size_t bytes;};
enum pt_midi_setup_owner_state {PT_MIDI_OWNER_LIVE=1,PT_MIDI_OWNER_CLOSED,PT_MIDI_OWNER_HELD};
struct pt_midi_setup_owner_borrow {
    const struct pt_midi_setup_owner *owner;
    struct pt_midi_setup *setup;
    uint32_t serial;
};
struct pt_midi_setup_owner_status {
    size_t workspace_bytes,metadata_budget,metadata_bytes;
    unsigned commands,modal;
    enum pt_midi_setup_owner_state state;
};
/* MIDI-only serialized sidecar, NOT a document/action/output owner. Supply
 * stationary zeroed aligned storage; only sizeof(owner) prefix is scanned or
 * written, while the entire declared capacity is protected against aliases.
 * The complete original allocator/context extent is genuinely known by caller;
 * its span contains allocator.context and stays disjoint from owner/workspace/
 * editor. Context may hold pointers to those objects, not contain the objects.
 * Unknown transitive allocations are NOT inferred from this declared span.
 * Fresh aligned disjoint living allocations remain an allocator precondition.
 * Callbacks must not mutate/destroy sources or retain transient arguments.
 * Public reentry refuses BUSY; arbitrary opaque-memory mutation is not detected.
 * Source snapshots compare selected shape/journal/allocator metadata, not every
 * project/PCM byte or arbitrary source lifetime changes.
 * Constructor NULL allocation returns CAPACITY with owned prefix zero, no child.
 * A known alias/unaligned/uncertain return is retained HELD without initializing
 * or releasing it as fresh memory. Original context/editor/document remain live.
 */
size_t pt_midi_setup_owner_size(void);
size_t pt_midi_setup_owner_alignment(void);
/* Callback-free negative transition admission, with no owner/output writes.
 * Exact constructor capacity and the SAME original initialized editor are
 * required; LIVE, no modal borrow, no draft and current genuine child binding.
 * STORAGE spans are disjoint from complete owner, context and workspace.
 * CONTEXT_INPUT permits only the exact captured complete allocator/context
 * extent as a shared input; a distinct disjoint context is also allowed. It
 * never permits an overlapping superset/subspan or any write over that context.
 * Span descriptors themselves remain disjoint read inputs.
 * No quiet/close/history-release or arbitrary allocator/heap census authority.
 * Original sources are still live here; do not call after editor_dispose.
 */
enum pt_midi_setup_owner_transition_role {PT_MIDI_OWNER_TRANSITION_STORAGE=0,
    PT_MIDI_OWNER_TRANSITION_CONTEXT_INPUT};
struct pt_midi_setup_owner_transition_span {
    const void *data;size_t bytes;enum pt_midi_setup_owner_transition_role role;
};
enum pt_midi_setup_result pt_midi_setup_owner_transition_admit(
    const struct pt_midi_setup_owner *,size_t,const struct pt_editor *,
    const struct pt_midi_setup_owner_transition_span *,unsigned);
/* Numeric known-storage exclusion for existing document output vetoes, including
 * after disposal. Does NOT read former editor/project/history or unknown journal
 * allocations. Exact original owner capacity and live sidecar/context/workspace
 * remain required; a false result grants no mutation/release authority. */
int pt_midi_setup_owner_storage_disjoint(const struct pt_midi_setup_owner *,size_t,
    const void *,size_t);
enum pt_midi_setup_result pt_midi_setup_owner_init(struct pt_midi_setup_owner *,size_t,
    struct pt_editor *,struct pt_midi_setup_owner_span,size_t metadata_budget);
/* Exactly one modal borrow. Ticket names the original allocator-owned setup,
 * never a requester-stack controller. Do not retain it beyond matched return.
 * Copies are observational tickets, not independent lifetime/release authority.
 * Within ONE original sidecar lifetime serial never wraps; wrong/replayed
 * tickets cannot consume another current borrow under that protocol. ALL ticket
 * copies end before sidecar release/zero/reinit. Same-address owner/workspace
 * reuse after a new constructor can repeat serial1; no global ABA or stale-
 * lifetime authentication is claimed. Raw pointer/serial is no lifetime pin.
 * Ticket/status buffers must also be disjoint from all retained journal records
 * and any otherwise unknown caller storage: known-span checks are no heap census.
 */
enum pt_midi_setup_result pt_midi_setup_owner_borrow(struct pt_midi_setup_owner *,
    struct pt_midi_setup_owner_borrow *);
/* Matched return cancels only the genuine draft, leaving committed/redo records.
 * Actual controller HELD ends this UI borrow but retains the whole sidecar/domain;
 * it never clears that controller's draft/uncertain obligation as recovery. */
enum pt_midi_setup_result pt_midi_setup_owner_return(struct pt_midi_setup_owner *,
    const struct pt_midi_setup_owner_borrow *);
enum pt_midi_setup_result pt_midi_setup_owner_cancel_draft(struct pt_midi_setup_owner *);
/* Direct actual close is valid after editor_dispose: it reads only controller
 * ownership. IN_USE retains original workspace/allocator; no history clear or
 * supplied closed/quiet flag. OK consumes the controller, NOT its workspace. */
enum pt_midi_setup_result pt_midi_setup_owner_close_controller(struct pt_midi_setup_owner *);
/* Only after positive close, with no ticket/draft/record surviving. Caller
 * supplies a genuinely initialized final-address editor from existing lifecycle;
 * this function does not publish/copy a document. Same original allocator only.
 * It zeroes/reuses the exact still-owned workspace then calls genuine setup init.
 * A failed init retains HELD; it never restores an old editor/controller image. */
enum pt_midi_setup_result pt_midi_setup_owner_rebind(struct pt_midi_setup_owner *,struct pt_editor *);
/* CLOSED-only original workspace free, consumed once before callback reentry.
 * No former editor/project read; no HELD recovery. Caller owns sidecar storage.
 * Original allocator context lives until this returns and all journal records end. */
enum pt_midi_setup_result pt_midi_setup_owner_release_workspace(struct pt_midi_setup_owner *);
enum pt_midi_setup_result pt_midi_setup_owner_get(struct pt_midi_setup_owner *,
    struct pt_midi_setup_owner_status *);
#endif
