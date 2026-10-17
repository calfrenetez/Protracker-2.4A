#ifndef PT_EDITOR_DOCUMENT_ACTION_H
#define PT_EDITOR_DOCUMENT_ACTION_H
#include "editor_document_replacement.h"
#include "mixed_owner_state_internal.h"
#define PT_DOCUMENT_ACTION_CONTEXTS 4U
#define PT_DOCUMENT_ACTION_SPANS 64U
/* Private serialized opt-in HOST action. The zero action, empty detached candidate, genuine
 * old owner, all named contexts and byte source stay alive until finish/cancel.
 * NEW/load prepare a detached candidate before any STOP. Cancellation is only
 * available before the first step. Existing typed replacement owns retirement
 * and final-address owner construction; no caller release grant exists here.
 * Allocators still promise fresh aligned extents and callbacks cannot destroy
 * or edit arbitrary storage. Observed reentry/staleness retains HOLD, not retry.
 * FAILED/CANCELED retain result and the precise core preparation_result after
 * confirmed cleanup; self is NULL and no ownership is granted by that report.
 * Reset the ended genuine action before another begin. Reset refuses adopted
 * or uncertain controls and never frees or authorizes document release.
 * No native/UI integration or timing/placement/physical acceptance follows.
 */
struct pt_editor_document_action_inputs {
    struct pt_document *document,*candidate;
    struct pt_editor_mixed *binding;
    struct pt_mixed_established *old_control,*new_control;
    struct pt_paula_voices *old_paula;
    struct pt_wavetable_voices *old_amigus;
    struct pt_editor_mixed_establish *establish;
    struct pt_editor_mixed_bridges *bridges;
    struct pt_editor_mixed_bridge_inputs bridge_inputs;
    struct pt_render_options options;
    struct pt_paula_render_caps caps;
    struct pt_playback_format format;
    struct pt_allocator allocator;
    struct pt_sampler_storage_span contexts[PT_DOCUMENT_ACTION_CONTEXTS];
    unsigned context_count,work;
#if defined(PT_DOCUMENT_MIDI_SETUP_HOST_ONLY)
    /* Optional genuine original sidecar, still live during prepare/cancel. No
     * retained modal ticket/draft; exact constructor capacity, not a prefix.
     * Caller keeps action/candidate/byte source/contexts disjoint from all
     * opaque retained MIDI records; fresh disjoint allocator contract remains.
     * Known sidecar exclusions do not infer a complete resource-span census.
     * Keep original sidecar/workspace/context alive through genuine finish;
     * final workspace release is a later explicit owner operation, not finish. */
    struct pt_midi_setup_owner *midi_setup;size_t midi_capacity;
#endif
};
enum pt_document_action_result {
    PT_DOCUMENT_ACTION_INVALID,PT_DOCUMENT_ACTION_FAILED,PT_DOCUMENT_ACTION_PREPARED,
    PT_DOCUMENT_ACTION_CANCELED,PT_DOCUMENT_ACTION_WAITING,PT_DOCUMENT_ACTION_RETIRING,
    PT_DOCUMENT_ACTION_PUBLISHED,PT_DOCUMENT_ACTION_COMMITTED,
    PT_DOCUMENT_ACTION_COMMITTED_NO_OWNER,PT_DOCUMENT_ACTION_HOLD
};
struct pt_editor_document_action {
    struct pt_editor_document_action *self;
    struct pt_editor_document_replacement replacement;
    struct pt_editor_document_action_inputs inputs;
    struct pt_document old_document;
    struct pt_editor old_editor;
    struct pt_editor_mixed old_binding;
    struct pt_mixed_established old_control;
    struct pt_paula_voices old_paula;
    struct pt_wavetable_voices old_amigus;
    struct pt_sampler_paula old_paula_cache;
    struct pt_sampler_wavetable old_amigus_cache;
    struct pt_mixed_owner old_owner;
    struct pt_amigus_wavetable_cache old_backend;
    struct pt_amigus_reservation old_reservation;
    struct pt_sampler_storage_span protected[PT_DOCUMENT_ACTION_SPANS];
    unsigned protected_count,phase,busy,fault,cleanup_attempted;
    const uint8_t *source;size_t source_bytes;
    enum pt_project_result preparation_result;
    enum pt_document_action_result result;
};
enum pt_document_action_result pt_editor_document_action_new(
    struct pt_editor_document_action *,const struct pt_editor_document_action_inputs *,unsigned,size_t);
enum pt_document_action_result pt_editor_document_action_load(
    struct pt_editor_document_action *,const struct pt_editor_document_action_inputs *,const uint8_t *,size_t,size_t);
enum pt_document_action_result pt_editor_document_action_step(struct pt_editor_document_action *);
enum pt_document_action_result pt_editor_document_action_cancel(struct pt_editor_document_action *);
int pt_editor_document_action_finish(struct pt_editor_document_action *);
int pt_editor_document_action_reset(struct pt_editor_document_action *);
#endif
