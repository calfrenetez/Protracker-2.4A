#ifndef PT_NATIVE_MIDI_REQUESTER_H
#define PT_NATIVE_MIDI_REQUESTER_H
#include "../editor/midi_setup.h"

#define PT_MIDI_REQUESTER_WIDTH 552
#define PT_MIDI_REQUESTER_HEIGHT 258
#define PT_MIDI_REQUESTER_NOTICE "SAVED SETTINGS ONLY / CAMD INACTIVE"
enum pt_midi_requester_stage {PT_MIDI_REQUESTER_CHOOSE=1,PT_MIDI_REQUESTER_FIELD};
enum pt_midi_requester_focus {
    PT_MIDI_FOCUS_INPUT,PT_MIDI_FOCUS_OUTPUT,PT_MIDI_FOCUS_CLOCK,
    PT_MIDI_FOCUS_NAME,PT_MIDI_FOCUS_SEND,PT_MIDI_FOCUS_RECEIVE,
    PT_MIDI_FOCUS_APPLY,PT_MIDI_FOCUS_CANCEL,PT_MIDI_FOCUS_COUNT
};
enum pt_midi_requester_key {
    PT_MIDI_KEY_CHARACTER,PT_MIDI_KEY_TAB,PT_MIDI_KEY_BACKTAB,
    PT_MIDI_KEY_ACTIVATE,PT_MIDI_KEY_ESCAPE,PT_MIDI_KEY_BACKSPACE,
    PT_MIDI_KEY_DELETE,PT_MIDI_KEY_LEFT,PT_MIDI_KEY_RIGHT,
    PT_MIDI_KEY_HOME,PT_MIDI_KEY_END,PT_MIDI_KEY_CLEAR
};
enum pt_midi_requester_event {
    PT_MIDI_EVENT_NONE,PT_MIDI_EVENT_CHANGED,PT_MIDI_EVENT_APPLY,PT_MIDI_EVENT_CANCEL
};
struct pt_midi_requester_rect {int left,top,right,bottom;};
extern const struct pt_midi_requester_rect pt_midi_requester_controls[PT_MIDI_FOCUS_COUNT];
/* Original serialized, separately owned Task-local UI object. Its complete
 * storage must be disjoint from the complete original controller capacity and
 * all original editor, project/document tables/PCM, history buffers, retained
 * records and allocator/context storage; these sources stay alive throughout
 * each synchronous call. The numeric open check covers only the controller's
 * minimum opaque prefix, NOT arbitrary model/source spatial admission. Caller
 * placement establishes these full extents. The setup/track window wrapper uses
 * a separate Task-stack model; the external-model entry borrows the caller's
 * original separately placed model. No callback retains either. Zero-initialize;
 * do not copy, edit
 * or reinitialize a live
 * model, controller, editor, document or history. Only this model may own its
 * controller draft during the modal interval. No callback retains this model.
 * The controller itself is borrowed: it and its allocator context must outlive
 * ALL journal records after this model/window closes. Never allocate that owner
 * on the requester stack or replace the editor's existing change barrier.
 */
struct pt_midi_requester_model {
    struct pt_midi_requester_model *self;
    struct pt_midi_setup *setup;
    unsigned track,stage,focus,cursor,busy,held;
    enum pt_midi_setup_target target;
    enum pt_midi_setup_result last_result;
    uint32_t flags;
    char name[PT_MIDI_ENDPOINT];
};
/* Opens the silent chooser; it neither begins a controller draft nor queries
 * drivers. track is captured once, not a later editor selection. Selection
 * refreshes copied saved values before genuine begin. No inventory is invented.
 */
enum pt_midi_setup_result pt_midi_requester_model_open(struct pt_midi_requester_model *,
    struct pt_midi_setup *,unsigned track);
enum pt_midi_setup_result pt_midi_requester_model_select(struct pt_midi_requester_model *,
    enum pt_midi_setup_target);
unsigned pt_midi_requester_hit(const struct pt_midi_requester_model *,int,int);
int pt_midi_requester_model_focus(struct pt_midi_requester_model *,unsigned);
enum pt_midi_requester_event pt_midi_requester_model_key(struct pt_midi_requester_model *,
    enum pt_midi_requester_key,unsigned character);
/* Apply pushes one field's local text/flags to the already-owned genuine draft
 * and commits one field. OK includes semantic no-op; it consumes this model.
 * Refusal retains the draft for correction or Cancel. No automatic Stop/retry.
 * HELD can close the UI, but MUST retain the durable original controller/context.
 */
enum pt_midi_setup_result pt_midi_requester_model_apply(struct pt_midi_requester_model *);
/* Discards only this model's genuine draft. HELD closes the model without any
 * controller cleanup; original unresolved ownership remains retained. */
enum pt_midi_setup_result pt_midi_requester_model_cancel(struct pt_midi_requester_model *);

struct Window;
enum pt_midi_requester_result {
    PT_MIDI_REQUESTER_HELD=-3,PT_MIDI_REQUESTER_REFUSED=-2,
    PT_MIDI_REQUESTER_UNAVAILABLE=-1,PT_MIDI_REQUESTER_CANCELLED,
    PT_MIDI_REQUESTER_ACCEPTED
};
/* Separate fixed window/UserPort. Caller owns normal-Task admission with idle
 * audio/capture/workflow, parent input isolation/drain/restoration and redraw.
 * This function never services a
 * timer/editor dispatch or opens/connects/sends through CAMD. It does not own the
 * controller. ACCEPTED includes unchanged values. On HELD retain its original
 * controller/allocator/editor/journal contexts; no forced history clearing.
 * Native window source is a candidate, not a qualified frontend integration.
 */
enum pt_midi_requester_result pt_native_midi_requester(struct Window *,
    struct pt_midi_setup *,unsigned track);
/* Private successor: display the caller's genuine already-open chooser at its
 * original address. It may be the actual model inside an original modal binding;
 * pass no copy. No reopen/reset/ownership transfer occurs. Refused admission and
 * GUI-unavailable return leave the model unchanged. Admission requires a live
 * chooser, no busy/HELD or controller draft, and the captured track/current saved
 * source query; it is not arbitrary-pointer/source spatial admission.
 * Caller supplies normal serialized Task execution, complete supported model/
 * owner/editor/project/history/records/allocator lifetimes and disjoint storage,
 * also disjoint from all borrowed parent Window/Screen/RPort/UserPort storage.
 * Parent input isolation/drain/restoration/redraw remains caller work, not done
 * here. No other task, callback or parent dispatch may change these live objects.
 * Apply/Cancel may consume this actual model. The window closes only its own
 * font/window; UNAVAILABLE does not cancel it. Any genuine modal ticket remains
 * caller-owned across every result: return it once through the genuine modal API
 * after this call, retaining the whole domain on uncertainty/HELD. No controller/
 * owner close, workspace release, history disposal, Stop or CAMD action occurs.
 * Source candidate only: this declares no native owner/window/frontend admission.
 */
enum pt_midi_requester_result pt_native_midi_requester_model_window(struct Window *,
    struct pt_midi_requester_model *);
#if defined(PT_MIDI_REQUESTER_OWNER_HOST_ONLY) || defined(PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY) || defined(PT_MIDI_SESSION_REQUESTER_NATIVE_GUI_QUALIFICATION_ONLY)
/* Separate unselected native GUI qualification, never inferred from RAM flags. */
#if defined(PT_MIDI_SESSION_REQUESTER_NATIVE_GUI_QUALIFICATION_ONLY)
#if PT_MIDI_SESSION_REQUESTER_NATIVE_GUI_QUALIFICATION_ONLY != 1 || !defined(__m68k__) || !defined(__amigaos__) || defined(__AROS__) || defined(__MORPHOS__)
#error "Session requester GUI qualification requires value1 and the genuine classic target"
#endif
#if defined(PT_MIDI_FRONTEND_RECORDING_SESSION_HOST_ONLY) || defined(PT_MIDI_FRONTEND_CONTEXT_HOST_ONLY) || defined(PT_MIDI_SETUP_OWNER_HOST_ONLY) || defined(PT_MIDI_REQUESTER_OWNER_HOST_ONLY) || defined(PT_MIDI_SESSION_REQUESTER_HOST_MODEL_TEST_ONLY) || defined(PT_MIDI_FRONTEND_RECORDING_SESSION_NATIVE_TASK_PROBE_ONLY) || defined(PT_MIDI_FRONTEND_CONTEXT_NATIVE_TASK_PROBE_ONLY) || defined(PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY) || defined(PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY) || defined(PT_EDITOR_MIDI_RECORDING_NATIVE_TASK_PROBE_ONLY) || defined(PT_DOCUMENT_MIDI_SETUP_HOST_ONLY) || defined(PT_DOCUMENT_MIDI_SETUP_NATIVE_TASK_PROBE_ONLY) || defined(PT_DOCUMENT_TYPED_REPLACEMENT) || defined(PT_DOCUMENT_TYPED_ACTION)
#error "Session requester GUI qualification excludes HOST/RAM/document selectors"
#endif
#else
#if defined(PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY)
#if PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY != 1 || !defined(PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY) || PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY != 1 || defined(PT_MIDI_REQUESTER_OWNER_HOST_ONLY)
#error "Requester modal native partition is the paired TASK RAM probe only"
#endif
#else
#if PT_MIDI_REQUESTER_OWNER_HOST_ONLY != 1
#error "Requester modal composition requires explicit HOST-only selection"
#endif
#endif
#endif /* separate native GUI qualification */
/* The owner header keeps its old default/HOST gates and independently checks
 * the exact genuine native TASK-only qualification partition when selected. */
#include "../editor/midi_setup_owner.h"
/* Original zeroed, aligned, stationary, separately owned modal binding. The
 * complete binding, not just its model, must avoid complete original owner/
 * workspace/allocator-context storage before any write. The admission query
 * checks those known extents, not a transitive heap census. Caller additionally
 * places the complete binding apart from original editor/project/document/PCM,
 * history buffers, retained MIDI records and all other live inputs. These and
 * the original allocator context remain live through matched return and every
 * retained record. No live copy/reinit/mutation or same-address lifetime reuse.
 * Only model editing APIs may change model while borrowed. Do not copy/replace
 * its ticket or call owner return directly. This object owns no controller,
 * journal, heap block, output, document or quiet/teardown authority.
 */
struct pt_midi_requester_modal {
    struct pt_midi_requester_modal *self;
    struct pt_midi_setup_owner *owner;
    struct pt_midi_setup_owner_borrow ticket;
    struct pt_midi_requester_model model;
    unsigned busy,held,return_attempted;
};
/* Callback-free begin validates the exact original owner capacity/editor and
 * full binding extent, obtains its real modal ticket, then opens the existing
 * chooser with once-captured track. No implicit Stop, allocation or driver work.
 * A failed model open returns that genuine ticket once. Positive return zeroes
 * only this unadopted binding; failure/HELD retains it and the entire domain.
 */
enum pt_midi_setup_result pt_midi_requester_modal_begin(struct pt_midi_requester_modal *,
    struct pt_midi_setup_owner *,size_t,struct pt_editor *,unsigned track);
/* Ends an actual live model, then returns the exact original ticket ONCE.
 * Apply may already have consumed model; committed and undone journal commands
 * remain owned by the original durable controller. OK zeroes only the transient
 * binding, so it may be opened again. Any uncertain refusal/HELD latches held,
 * retains original ticket/domain and forbids retry/reset/free/closure inference.
 * During real model callbacks BUSY refuses without touching either owner/model.
 * No post-dispose call is supported; return before any document transition.
 */
enum pt_midi_setup_result pt_midi_requester_modal_return(struct pt_midi_requester_modal *);
#endif
#endif
