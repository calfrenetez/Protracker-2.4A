#ifndef PT_MIDI_FRONTEND_RECORDING_SESSION_H
#define PT_MIDI_FRONTEND_RECORDING_SESSION_H
#if !defined(PT_MIDI_FRONTEND_RECORDING_SESSION_HOST_ONLY) || PT_MIDI_FRONTEND_RECORDING_SESSION_HOST_ONLY != 1
#error "Recording session requires its explicit HOST-only selection"
#endif
#if defined(__m68k__) || defined(__amigaos__) || defined(__AROS__) || defined(__MORPHOS__)
#error "Recording session has no native or frontend admission"
#endif
#include "midi_frontend_context.h"
#include "../editor/editor_midi_recording.h"
struct pt_midi_recording_session;
enum pt_midi_recording_session_state {
 PT_MIDI_SESSION_INVALID=0,PT_MIDI_SESSION_CONSTRUCTING,PT_MIDI_SESSION_LIVE,
 PT_MIDI_SESSION_MODAL,PT_MIDI_SESSION_TAKE,PT_MIDI_SESSION_CLOSED,PT_MIDI_SESSION_HELD
};
/* HOST-only typed composition, not an input source, scheduler or output owner.
 * One serialized original caller owns separate zero/aligned session, frontend,
 * sidecar and take storage with truthful complete capacities. They and original
 * editor/project/history/records/allocator context stay stationary and disjoint.
 * The supplied complete context starts exactly at original sampler.allocator.context;
 * an enclosing span with an interior context pointer is not admitted here.
 * Unknown transitive objects remain caller obligations, not a heap census.
 * The ORIGINAL allocator promises fresh aligned disjoint living allocations,
 * including separation from these supplied storage objects. No direct use/copy/
 * reset/reinit of adopted child controls is permitted. All operational aliases
 * belong exclusively to this session; pointer checks are not ABA authentication.
 * Callbacks cannot mutate/destroy original sources or retain transient inputs.
 *
 * Construct delegates to the genuine frontend constructor. Ordinary preflight
 * and genuine NULL allocation refuse without adopting any owner; uncertainty is
 * terminal HELD and retains the whole caller domain. No Stop/quiet is inferred.
 * Exact genuine original owner/editor/full-capacity admission is checked before
 * each take operation and after callbacks; original recorded journal revisions
 * remain valid, while unexpected modal/draft/allocator/controller drift retains
 * the whole domain HELD. No frozen constructor revision is used as authority.
 * Setup modal and recording are exclusive. While a take is attached, including
 * stopped/canceled-before-detach, setup and shutdown refuse before child calls.
 * Cancel only delegates LOCAL recorder cancellation: not detach or device quiet.
 * Stop failures retain obligations. Only positive genuine detach ends a take;
 * its final status is observed locally before clearing ONLY that ended prefix.
 * Tail bytes survive. Every old take alias must end at this reuse boundary.
 * A later explicit record_begin starts a new genuine lifetime on the same zero
 * workspace; no old ticket/pointer is valid as authority for that new take.
 *
 * Final shutdown delegates actual frontend editor-dispose/close/workspace-release
 * once, after caller finishes original playback/cache/voice shutdown. It never
 * frees caller objects/document/UI or authorizes unrelated teardown. HELD has no
 * forced cleanup/retry/reset. CLOSED state and refused operations touch only
 * local/numeric known spans, never former source objects or allocator context.
 */
size_t pt_midi_recording_session_size(void);
size_t pt_midi_recording_session_alignment(void);
enum pt_midi_setup_result pt_midi_recording_session_construct(
 struct pt_midi_recording_session *,size_t,struct pt_midi_frontend_context *,size_t,
 struct pt_midi_setup_owner *,size_t,struct pt_editor_midi_recording *,size_t,
 struct pt_editor *,struct pt_midi_setup_owner_span,size_t metadata_budget);
enum pt_midi_setup_result pt_midi_recording_session_setup_begin(struct pt_midi_recording_session *,unsigned);
struct pt_midi_requester_model *pt_midi_recording_session_model(struct pt_midi_recording_session *);
enum pt_midi_setup_result pt_midi_recording_session_setup_end(struct pt_midi_recording_session *);
enum pt_editor_midi_record_result pt_midi_recording_session_record_begin(
 struct pt_midi_recording_session *,const struct pt_record_settings *,uint64_t,unsigned,unsigned);
enum pt_editor_midi_record_result pt_midi_recording_session_message(
 struct pt_midi_recording_session *,uint64_t,const uint8_t *,size_t);
enum pt_editor_midi_record_result pt_midi_recording_session_stop(struct pt_midi_recording_session *,uint64_t);
enum pt_editor_midi_record_result pt_midi_recording_session_cancel(struct pt_midi_recording_session *);
enum pt_editor_midi_record_result pt_midi_recording_session_detach(struct pt_midi_recording_session *);
/* Attached-current-only observation; no output writes outside a genuine TAKE.
 * Known control/context/source aliases refuse. Buffers must also be disjoint
 * from opaque retained MIDI records and other caller inputs under their complete
 * lifetime contract. No detached external traversal. */
enum pt_editor_midi_record_result pt_midi_recording_session_record_status(
 struct pt_midi_recording_session *,struct pt_editor_midi_record_status *);
enum pt_midi_setup_result pt_midi_recording_session_dispose_close(struct pt_midi_recording_session *);
enum pt_midi_recording_session_state pt_midi_recording_session_state(const struct pt_midi_recording_session *);
#endif
