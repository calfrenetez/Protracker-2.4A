#ifndef PT_EDITOR_DOCUMENT_REPLACEMENT_H
#define PT_EDITOR_DOCUMENT_REPLACEMENT_H
#include "editor_mixed_checked.h"
#include "editor_mixed_establish.h"
#include "editor_mixed_bridges.h"
#include "../core/document.h"
/* Separate ordinary-Task RAM qualification only; no frontend/publication grant.
 * Existing default/HOST selections and their bodies remain unchanged. */
#if defined(PT_DOCUMENT_MIDI_SETUP_NATIVE_TASK_PROBE_ONLY)
#if PT_DOCUMENT_MIDI_SETUP_NATIVE_TASK_PROBE_ONLY != 1 || defined(PT_DOCUMENT_MIDI_SETUP_HOST_ONLY) || defined(PT_MIDI_SETUP_OWNER_HOST_ONLY) || defined(PT_MIDI_REQUESTER_OWNER_HOST_ONLY)
#error "Native MIDI document TASK RAM probe excludes HOST composition"
#endif
#if !defined(PT_DOCUMENT_TYPED_ACTION) || PT_DOCUMENT_TYPED_ACTION != 1 || !defined(PT_DOCUMENT_TYPED_REPLACEMENT) || PT_DOCUMENT_TYPED_REPLACEMENT != 1 || !defined(PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY) || PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY != 1 || !defined(PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY) || PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY != 1
#error "Native MIDI document TASK RAM probe requires exact typed and owner options"
#endif
#if !defined(__m68k__) || !defined(__amigaos__) || defined(__AROS__) || defined(__MORPHOS__)
#error "Native MIDI document TASK RAM probe requires the genuine classic target"
#endif
#endif
#if defined(PT_DOCUMENT_MIDI_SETUP_HOST_ONLY) || defined(PT_DOCUMENT_MIDI_SETUP_NATIVE_TASK_PROBE_ONLY)
#if defined(PT_DOCUMENT_MIDI_SETUP_HOST_ONLY) && PT_DOCUMENT_MIDI_SETUP_HOST_ONLY != 1
#error "MIDI document composition requires explicit HOST-only selection"
#endif
#include "midi_setup_owner.h"
#endif
/* Private serialized typed replacement. A candidate is a complete, detached,
 * genuinely owned document made by the ordinary document API while the old
 * document still lives. Its full-capacity allocations must be disjoint from
 * old storage and all living sampler/control allocations. No copy of a header
 * conveys allocation ownership; fresh allocator extent/lifetime is a contract.
 * One private replacement may be active at a time. Begin is callback-free. Controls, candidate, contexts, old and new engines,
 * backend/reservation and allocators remain alive until confirmed completion.
 * Never copy/edit the control or source arrays while adopted.
 *
 * Stop/disposal are irreversible. RETIRING is an uncommitted storage HOLD: it
 * retains old document allocations and candidate ownership, allows only later
 * retirement observation after real pin release, and does not permit use of a
 * potentially retired expanded project table. WAITING retains old allocations but
 * does not promise the old editor remains playable. Once PUBLISHED is reached,
 * the new document is retained even if establishing masters or creating its
 * final-address owner fails. COMMITTED_NO_OWNER means confirmed quiet; HOLD
 * keeps the exact controls/allocations alive and never authorizes forced cleanup.
 * This opt-in path does not change automatic load/new/release or editor PLAY.
 * Finish also requires the final sampler storage retired, after confirmed new
 * owner/cache close and editor disposal. HOLD has no forced finish/retry grant.
 * Finish also keeps the document veto until the complete four-control bridge
 * object is zero. This negative check is not independent closure authority.
 * The captured sampler budget includes the actual loaded-source debit, checked
 * for overflow before stop. Dispose must genuinely empty the old source and
 * song ownership; a void source release or budget credit alone is insufficient.
 * Only the original song allocator/budget policy is restored on the fresh empty
 * editor. No old source/song owner, history, allocation or generation is copied.
 * It is not native, device, placement, latency or factory-membership authority.
 */
enum pt_document_replace_result {
    PT_DOCUMENT_REPLACE_INVALID,PT_DOCUMENT_REPLACE_PREPARED,
    PT_DOCUMENT_REPLACE_WAITING,PT_DOCUMENT_REPLACE_RETIRING,PT_DOCUMENT_REPLACE_PUBLISHED,
    PT_DOCUMENT_REPLACE_COMMITTED,PT_DOCUMENT_REPLACE_COMMITTED_NO_OWNER,
    PT_DOCUMENT_REPLACE_HOLD
};
struct pt_editor_document_replacement {
    struct pt_editor_document_replacement *self;
    struct pt_document *document,*candidate;
    struct pt_document old_header,next_header;
    struct pt_editor *editor;
    struct pt_editor_mixed *binding;
    struct pt_mixed_established *old_control,*new_control;
    struct pt_paula_voices *old_paula;
    struct pt_wavetable_voices *old_amigus;
    struct pt_editor_mixed_bridges *new_bridges;
    struct pt_editor_mixed_bridge_inputs inputs;
    struct pt_render_options options;
    struct pt_paula_render_caps caps;
    struct pt_playback_format format;
    struct pt_allocator sampler_allocator,song_allocator;
    struct pt_sampler_storage_span context,parent;
    struct pt_editor_mixed_establish *establish;
    void *protected_storage[14];size_t protected_bytes[14];
    size_t sampler_budget,song_budget;
#if defined(PT_DOCUMENT_MIDI_SETUP_HOST_ONLY) || defined(PT_DOCUMENT_MIDI_SETUP_NATIVE_TASK_PROBE_ONLY)
    struct pt_midi_setup_owner *midi_setup;size_t midi_capacity;
    unsigned midi_close_attempted,midi_rebind_attempted;
#endif
    unsigned work,phase,busy,fault,release_attempted;
    enum pt_document_replace_result result;
};
enum pt_document_replace_result pt_editor_document_replacement_begin(
    struct pt_editor_document_replacement *,struct pt_document *,struct pt_document *,
    struct pt_editor_mixed *,struct pt_mixed_established *,
    struct pt_paula_voices *,struct pt_wavetable_voices *,
    struct pt_editor_mixed_establish *,struct pt_mixed_established *,
    struct pt_editor_mixed_bridges *,const struct pt_editor_mixed_bridge_inputs *,
    const struct pt_render_options *,const struct pt_paula_render_caps *,
    const struct pt_playback_format *,struct pt_sampler_storage_span,unsigned);
#if defined(PT_DOCUMENT_MIDI_SETUP_HOST_ONLY) || defined(PT_DOCUMENT_MIDI_SETUP_NATIVE_TASK_PROBE_ONLY)
/* Optional HOST-only enrollment on this genuine PREPARED phase0 replacement.
 * It neither closes nor cancels anything. Original owner must match this editor;
 * full capacity is verified by its typed query. NULL preserves ordinary behavior.
 * Enrollment/close/rebind refusal latches HOLD without retry. Fresh rebind runs
 * only after publication, final editor_init and original sampler policy restore.
 * Empty source policy and repeating-session finish limits are unchanged. The
 * original song allocator/budget is restored only on the fresh empty owner.
 * The context/parent containing this mutable control is STORAGE, never an
 * allocator-context-input exemption. All controls/candidate/contexts remain
 * disjoint from opaque retained MIDI records under the original placement and
 * fresh-disjoint allocator preconditions; no complete resource census exists.
 * Sidecar/workspace/context stay alive through genuine finish; the final owner
 * must be positively closed before its separate explicit workspace release. */
enum pt_document_replace_result pt_editor_document_replacement_midi_setup(
    struct pt_editor_document_replacement *,struct pt_midi_setup_owner *,size_t);
#endif
enum pt_document_replace_result pt_editor_document_replacement_step(
    struct pt_editor_document_replacement *);
int pt_editor_document_replacement_finish(struct pt_editor_document_replacement *);
#endif
