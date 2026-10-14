#ifndef PT_EDITOR_DOCUMENT_REPLACEMENT_H
#define PT_EDITOR_DOCUMENT_REPLACEMENT_H
#include "editor_mixed_checked.h"
#include "editor_mixed_establish.h"
#include "editor_mixed_bridges.h"
#include "../core/document.h"
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
    struct pt_allocator sampler_allocator;
    struct pt_sampler_storage_span context,parent;
    struct pt_editor_mixed_establish *establish;
    void *protected_storage[14];size_t protected_bytes[14];
    size_t sampler_budget;
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
enum pt_document_replace_result pt_editor_document_replacement_step(
    struct pt_editor_document_replacement *);
int pt_editor_document_replacement_finish(struct pt_editor_document_replacement *);
#endif
