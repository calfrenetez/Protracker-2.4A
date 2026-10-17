#ifndef PT_MIDI_SESSION_REQUESTER_H
#define PT_MIDI_SESSION_REQUESTER_H
/* An unselected experiment. Neither TASK RAM flags nor a default frontend can
 * admit this GUI caller. HOST selects protocol declarations/model tests only. */
#if defined(PT_MIDI_SESSION_REQUESTER_HOST_MODEL_TEST_ONLY)
#if PT_MIDI_SESSION_REQUESTER_HOST_MODEL_TEST_ONLY != 1 || defined(PT_MIDI_SESSION_REQUESTER_NATIVE_GUI_QUALIFICATION_ONLY)
#error "Session requester requires its exclusive HOST model selection"
#endif
#else
#if !defined(PT_MIDI_SESSION_REQUESTER_NATIVE_GUI_QUALIFICATION_ONLY) || PT_MIDI_SESSION_REQUESTER_NATIVE_GUI_QUALIFICATION_ONLY != 1
#error "Session requester requires the distinct native GUI qualification selection"
#endif
#endif
#include "midi_frontend_recording_session.h"
#include "midi_requester_parent.h"
struct pt_midi_session_requester_domain {
 struct pt_midi_recording_session *session;size_t session_capacity;
 struct pt_midi_frontend_context *frontend;size_t frontend_capacity;
 struct pt_midi_setup_owner *owner;size_t owner_capacity;
 struct pt_editor_midi_recording *take;size_t take_capacity;
 struct pt_editor *editor;struct pt_midi_setup_owner_span allocator_context;
};
enum pt_midi_session_requester_result {
 PT_MIDI_SESSION_REQUESTER_REFUSED=-2,PT_MIDI_SESSION_REQUESTER_BUSY=-1,
 PT_MIDI_SESSION_REQUESTER_UNUSED=0,PT_MIDI_SESSION_REQUESTER_COMPLETE,
 PT_MIDI_SESSION_REQUESTER_HELD
};
struct pt_midi_session_requester_progress {
 struct pt_midi_session_requester_progress *self;size_t capacity;
 struct pt_midi_session_requester_domain domain;
 struct Window *original_parent;size_t parent_bytes;unsigned track;
 pt_midi_parent_repaint repaint;void *repaint_context;size_t repaint_bytes;
 struct ExecBase *exec;struct Task *task;struct Library *intuition;
 struct pt_midi_requester_model *captured_model;
 enum pt_midi_session_requester_result state;unsigned busy;
 unsigned begin_attempted,model_captured,parent_attempted,end_attempted,restored_checked;
 enum pt_midi_setup_result begin_result,end_result;
 struct pt_midi_requester_parent_progress parent_progress;
};
/* ONE synchronous use of separately owned zero/aligned stationary progress.
 * Full capacities and context extents must be truthful original constructor
 * inputs; this descriptor names the SAME original session/frontend/owner/take/
 * initialized editor. This SAME-binding identity is a truthful original-caller
 * precondition, not authenticated across all opaque controls by the query on
 * the supplied owner alone. It is not another operational owner or ticket.
 * Progress, descriptor and complete original control/source/project/PCM/history/
 * retained record/allocator/GUI/repaint storage are disjoint where writable.
 * Numeric guards cover declared spans plus known minimum native GUI extents,
 * never a transitive heap census or arbitrary opaque-memory mutation detector.
 * Original allocator promises fresh disjoint living allocations. Unknown GUI
 * tails/records/inputs remain caller placement and lifetime obligations.
 *
 * Caller ALREADY owns the original normal serialized Task, private parent port/
 * signal, stationary original IntuitionV37+ domain and durable dispatch/modal
 * guard, and has suspended prepared display use. No concurrent callback/Task
 * may mutate/close sources or controls. These and the whole call chain survive
 * HELD. HOST model mode has no OS/Task/GUI authority and never compiles OS C.
 *
 * Attached TAKE (even stopped) refuses before UI. The genuine setup begins once;
 * its original model pointer is obtained ONCE before parent_window. Apply may
 * consume model.self. After positive parent RESTORED, genuine setup_end is still
 * attempted ONCE; no re-query/reopen/copy of the consumed model occurs. Only
 * positive matched end followed by parent_restored yields COMPLETE. No retry,
 * compensation, raw ticket return, owner close, disposal, free/reset or Stop.
 * Any post-borrow uncertainty retains HELD, including a GUI restored mask after
 * another fault. COMPLETE never clears caller guard or resumes drawing: caller
 * still performs actual redraw/resume under its existing lifecycle. No external
 * quiet, audio/device/input/CAMD/timing or frontend activation is established.
 * A valid repeated call observes only local progress and returns BUSY/HELD or
 * REFUSED; no former source is followed. End all aliases before fresh reuse.
 */
enum pt_midi_session_requester_result pt_native_midi_session_requester(
 struct pt_midi_session_requester_progress *,size_t,
 const struct pt_midi_session_requester_domain *,struct Window *,size_t,
 unsigned,pt_midi_parent_repaint,void *,size_t);
#endif
