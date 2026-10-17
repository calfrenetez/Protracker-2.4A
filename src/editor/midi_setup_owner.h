#ifndef PT_MIDI_SETUP_OWNER_H
#define PT_MIDI_SETUP_OWNER_H
#if !defined(PT_MIDI_SETUP_OWNER_HOST_ONLY) || PT_MIDI_SETUP_OWNER_HOST_ONLY != 1
#error "Private MIDI setup owner requires explicit HOST-only selection"
#endif
#if defined(__amigaos__) || defined(__m68k__) || defined(__AROS__) || defined(__MORPHOS__)
#error "This private MIDI setup owner has no native admission"
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
