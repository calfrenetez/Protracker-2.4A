#ifndef PT_NATIVE_MIDI_FRONTEND_CONTEXT_H
#define PT_NATIVE_MIDI_FRONTEND_CONTEXT_H
/* Qualification-only native branch. It admits this RAM probe, never the
 * shipping/frontend/window caller. Existing HOST selection remains below. */
#if defined(PT_MIDI_FRONTEND_CONTEXT_NATIVE_TASK_PROBE_ONLY)
#if PT_MIDI_FRONTEND_CONTEXT_NATIVE_TASK_PROBE_ONLY != 1 || !defined(PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY) || PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY != 1 || !defined(PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY) || PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY != 1
#error "Native frontend context requires the exact three TASK RAM probe options"
#endif
#if !defined(__m68k__) || !defined(__amigaos__) || defined(__AROS__) || defined(__MORPHOS__)
#error "Native frontend context TASK RAM probe requires the genuine classic target"
#endif
#if defined(PT_MIDI_FRONTEND_CONTEXT_HOST_ONLY) || defined(PT_MIDI_SETUP_OWNER_HOST_ONLY) || defined(PT_MIDI_REQUESTER_OWNER_HOST_ONLY) || defined(PT_DOCUMENT_MIDI_SETUP_HOST_ONLY) || defined(PT_DOCUMENT_MIDI_SETUP_NATIVE_TASK_PROBE_ONLY)
#error "Native frontend context TASK RAM probe excludes HOST/document/window composition"
#endif
#else
/* Private source composition only. None of the native TASK/RAM selectors admits
 * this caller or the shipping frontend. The genuine owner gate is unchanged. */
#if !defined(PT_MIDI_FRONTEND_CONTEXT_HOST_ONLY) || PT_MIDI_FRONTEND_CONTEXT_HOST_ONLY != 1
#error "MIDI frontend context requires its explicit HOST-only selection"
#endif
#if !defined(PT_MIDI_SETUP_OWNER_HOST_ONLY) || PT_MIDI_SETUP_OWNER_HOST_ONLY != 1 || !defined(PT_MIDI_REQUESTER_OWNER_HOST_ONLY) || PT_MIDI_REQUESTER_OWNER_HOST_ONLY != 1
#error "MIDI frontend context requires both genuine HOST owner/modal selections"
#endif
#if defined(__m68k__) || defined(__amigaos__) || defined(__AROS__) || defined(__MORPHOS__) || defined(PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY) || defined(PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY) || defined(PT_DOCUMENT_MIDI_SETUP_NATIVE_TASK_PROBE_ONLY)
#error "MIDI frontend context has no native or TASK-probe admission"
#endif
#endif
#include "midi_requester.h"

struct pt_midi_frontend_context;
enum pt_midi_frontend_state {
    PT_MIDI_FRONTEND_INVALID=0,PT_MIDI_FRONTEND_CONSTRUCTING,
    PT_MIDI_FRONTEND_LIVE,PT_MIDI_FRONTEND_MODAL,
    PT_MIDI_FRONTEND_CLOSED,PT_MIDI_FRONTEND_HELD
};
/* An ordinary caller, not this module, owns the complete context and separate
 * owner storage. Both must be zero, aligned, stationary, readable/writable,
 * genuinely owned and disjoint from each other, complete original editor,
 * project/document/PCM/history, all retained resource records and live inputs.
 * Numeric guards cover known extents; no arbitrary-resource or heap census.
 * The complete genuinely known allocator-context extent is disjoint from both
 * objects. It contains the editor's ORIGINAL sampler allocator.context and
 * remains live until all records and the original workspace release end.
 * That original allocator promises fresh aligned disjoint living allocations,
 * including separation from this newly supplied context. Its existing owner
 * constructor does not know/census this extra context when checking a returned
 * workspace alias. Violating that freshness contract is not safely detected.
 * Never copy/reset/reinitialize these objects or editor/project/history while
 * adopted. No same-address ABA authentication or cross-Task synchronization.
 * Callbacks are synchronous and must not mutate/destroy the original sources.
 */
size_t pt_midi_frontend_context_size(void);
size_t pt_midi_frontend_context_alignment(void);
/* Calls the REAL owner constructor, allocating only its genuine workspace.
 * Before that callback, known alias/invalid/nonzero inputs refuse without writes.
 * A genuine NULL allocation returns CAPACITY with both owned prefixes still zero.
 * Any uncertain/HELD construction retains the whole original caller domain.
 * Metadata records subsequently use the genuine separate byte budget; no master
 * or document allocator/accounting policy is changed here. */
enum pt_midi_setup_result pt_midi_frontend_context_construct(
    struct pt_midi_frontend_context *,size_t,
    struct pt_midi_setup_owner *,size_t,struct pt_editor *,
    struct pt_midi_setup_owner_span,size_t metadata_budget);
/* These are real owner tickets and the actual same-address requester model.
 * Apply can consume the model while the ticket still needs matched return.
 * Open failure may have already returned one real acquired ticket; do not repeat
 * a return. A positive end preserves committed/undone journal records. */
enum pt_midi_setup_result pt_midi_frontend_context_begin(
    struct pt_midi_frontend_context *,unsigned track);
struct pt_midi_requester_model *pt_midi_frontend_context_model(
    struct pt_midi_frontend_context *);
enum pt_midi_setup_result pt_midi_frontend_context_end(struct pt_midi_frontend_context *);
/* Explicit one-shot MIDI shutdown, NEVER implicit Stop/quiet or document release.
 * Caller must first finish genuine playback/voice/cache/recording shutdown under
 * its existing lifecycle, and keep original editor, sources and allocator alive.
 * This calls actual editor_dispose once, then genuine owner close and workspace
 * release once. Any refusal after the attempt latches HELD, no retry/reset/free.
 * No former editor/project traversal follows positive editor_dispose. CLOSED
 * proves only these MIDI operations. It grants no unrelated source/song/master,
 * document, UI, timer, sidecar-storage or context-storage release authority.
 * Those original owning lifecycles still decide their own retirement. */
enum pt_midi_setup_result pt_midi_frontend_context_dispose_close(
    struct pt_midi_frontend_context *);
/* Callback-free local observation; even CLOSED/HELD reads no former sources.
 * HELD is terminal. Its original caller must retain storage/call chain; this
 * portable module cannot park an OS Task or keep a returned stack frame alive. */
enum pt_midi_frontend_state pt_midi_frontend_context_state(
    const struct pt_midi_frontend_context *);
#endif
