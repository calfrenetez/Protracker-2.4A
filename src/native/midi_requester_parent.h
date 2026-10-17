#ifndef PT_NATIVE_MIDI_REQUESTER_PARENT_H
#define PT_NATIVE_MIDI_REQUESTER_PARENT_H
/* Unselected native source candidate, never a frontend/owner/Task admission. */
#if defined(PT_MIDI_SESSION_REQUESTER_HOST_MODEL_TEST_ONLY)
#if PT_MIDI_SESSION_REQUESTER_HOST_MODEL_TEST_ONLY != 1 || !defined(PT_MIDI_FRONTEND_RECORDING_SESSION_HOST_ONLY) || PT_MIDI_FRONTEND_RECORDING_SESSION_HOST_ONLY != 1 || !defined(PT_MIDI_FRONTEND_CONTEXT_HOST_ONLY) || PT_MIDI_FRONTEND_CONTEXT_HOST_ONLY != 1 || !defined(PT_MIDI_SETUP_OWNER_HOST_ONLY) || PT_MIDI_SETUP_OWNER_HOST_ONLY != 1 || !defined(PT_MIDI_REQUESTER_OWNER_HOST_ONLY) || PT_MIDI_REQUESTER_OWNER_HOST_ONLY != 1
#error "Parent protocol HOST model requires the exact existing four HOST selections"
#endif
#if defined(__m68k__) || defined(__amigaos__) || defined(__AROS__) || defined(__MORPHOS__) || defined(PT_MIDI_SESSION_REQUESTER_NATIVE_GUI_QUALIFICATION_ONLY) || defined(PT_MIDI_FRONTEND_RECORDING_SESSION_NATIVE_TASK_PROBE_ONLY) || defined(PT_MIDI_FRONTEND_CONTEXT_NATIVE_TASK_PROBE_ONLY) || defined(PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY) || defined(PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY) || defined(PT_DOCUMENT_MIDI_SETUP_HOST_ONLY) || defined(PT_DOCUMENT_MIDI_SETUP_NATIVE_TASK_PROBE_ONLY) || defined(PT_EDITOR_MIDI_RECORDING_NATIVE_TASK_PROBE_ONLY) || defined(PT_DOCUMENT_TYPED_ACTION) || defined(PT_DOCUMENT_TYPED_REPLACEMENT)
#error "Parent protocol HOST model excludes native/RAM/document admission"
#endif
#else
#if !defined(__m68k__) || !defined(__amigaos__) || defined(__AROS__) || defined(__MORPHOS__)
#error "MIDI parent bracket requires the genuine classic native target"
#endif
#endif /* HOST protocol declarations only; actual OS C is never selected */
#include "midi_requester.h"
struct ExecBase;struct Task;struct Library;struct MsgPort;struct Screen;
struct RastPort;struct Message;
#define PT_MIDI_PARENT_DRAIN_LIMIT 128U

enum pt_midi_parent_result {
    PT_MIDI_PARENT_REFUSED=-2,PT_MIDI_PARENT_BUSY=-1,PT_MIDI_PARENT_UNUSED=0,
    PT_MIDI_PARENT_RESTORED,PT_MIDI_PARENT_HELD
};
enum pt_midi_parent_phase {
    PT_MIDI_PARENT_CAPTURE,PT_MIDI_PARENT_ISOLATE,PT_MIDI_PARENT_PRE_DRAIN,
    PT_MIDI_PARENT_CHILD,PT_MIDI_PARENT_POST_DRAIN,PT_MIDI_PARENT_RESTORE,
    PT_MIDI_PARENT_COMPLETE
};
/* Completely repaint the original fixed SIMPLE_REFRESH parent using its
 * original RPort while BeginRefresh is active. Invalidate prepared backing
 * cache first. Synchronous: retain nothing, dispatch no input, change no model,
 * owner, sources or parent identities. Callback return is not pixel evidence.
 * Complete genuinely known context bytes remain live throughout HELD too. */
typedef void (*pt_midi_parent_repaint)(struct Window *,void *);
struct pt_midi_requester_parent_progress {
    struct pt_midi_requester_parent_progress *self;
    struct Window *parent;struct MsgPort *port;struct Screen *screen;
    struct RastPort *rport;struct ExecBase *exec;struct Task *task;
    struct Library *intuition;struct pt_midi_requester_model *model;
    pt_midi_parent_repaint repaint;void *repaint_context;size_t repaint_bytes;
    struct Message *pending_message;
    uint32_t original_idcmp;unsigned signal_bit;
    enum pt_midi_parent_result state;enum pt_midi_parent_phase phase;
    enum pt_midi_requester_result window_result;
    unsigned busy,isolation_attempted,isolation_confirmed,window_called;
    unsigned restore_attempted,restore_confirmed,pending_reply_attempted;
    unsigned refresh_active,get_calls[2],replied[2],repainted[2];
};
/* ONE use of zero, aligned, original noncopyable progress storage. Supported
 * objects are genuinely live, readable/writable and stationary: this is not
 * arbitrary-pointer admission. Progress is disjoint from the full model,
 * parent/port/screen/RPort/Task/Exec/library, controller/workspace, editor,
 * project/history/retained records and callback/input storage. Known minimum
 * extents are checked; there is no transitive allocation/alias census.
 *
 * Caller owns the original normal Task (IDNestCnt=TDNestCnt=-1), this separately
 * created unshared parent IDCMP port and signal, with NO other consumer,
 * dispatch, close or concurrent source/window mutation. Same Task/port fields
 * are observations, not proof of exclusive ownership. Supported parent: fixed
 * 640x512 at0,0, borderless/backdrop SIMPLE_REFRESH, no drag/size/depth/close
 * gadgets, RAWKEY|MOUSEBUTTONS|REFRESH|INACTIVE mask. IntuitionBase is a live
 * original V37+ library. Older BOOL semantics refuse before mutation. This
 * helper NEVER installs a zero IDCMP mask.
 *
 * BEFORE entry caller sets its durable dispatch/modal guard and suspends
 * prepared display use. Supply the actual already-open chooser (e.g. original
 * context_model result), never a copy/reopened model. Full controller/editor/
 * journal/allocator/GUI parents and repaint context stay live across this call
 * AND HELD. Do not mutate the progress object or borrowed sources in callbacks.
 *
 * Each pre/post drain has128 GetMsg attempts. Exhaustion conservatively holds
 * after replying the last owned message; no extra fetch proves empty. This is
 * a safety budget, not queue capacity or a time bound. Only full NT_MESSAGE
 * IntuiMessages naming this original parent are replied. A foreign/short/
 * deformed removed message remains verbatim in pending_message: no reply,
 * requeue, free or presumed ownership. Retain that obligation and domain.
 *
 * Uncertainty after isolation attempt is terminal HELD: no retry, compensating
 * restore, dispatch, cleanup or quiet inference. Pending-message/refresh flags
 * expose unfinished calls. The child services only its own port: parent
 * refresh may queue meanwhile. A HELD child skips further parent draining or
 * repaint; one trustworthy mask restoration may complete but never clears
 * HELD. Queued original parent messages then remain owned by its port.
 *
 * RESTORED confirms GUI mask/identities only at this boundary. Caller STILL
 * returns its ORIGINAL context/modal ticket exactly once, retains its guard
 * until that succeeds plus restored() below, then genuinely redraws/resumes.
 * This helper never returns an owner ticket, closes controller/workspace/
 * editor, clears caller guard or activates a window. Refused/unavailable child
 * open does not erase the caller's modal obligation. No storage reset/reuse
 * after HELD; RESTORED storage may be discarded/reused only after the complete
 * caller protocol ends. No cross-lifetime/ABA authentication is promised.
 */
enum pt_midi_parent_result pt_native_midi_requester_parent_window(
    struct pt_midi_requester_parent_progress *,struct Window *,
    struct pt_midi_requester_model *,pt_midi_parent_repaint,void *,size_t);
/* Pure observation; no OS mutation/ownership grant. Only while original GUI
 * parents remain live, after genuine matched owner return. False requires
 * retained caller HOLD, never a restore retry. */
int pt_native_midi_requester_parent_restored(
    const struct pt_midi_requester_parent_progress *);
#endif
