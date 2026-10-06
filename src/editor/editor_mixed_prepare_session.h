#ifndef PT_EDITOR_MIXED_PREPARE_SESSION_H
#define PT_EDITOR_MIXED_PREPARE_SESSION_H
#include "editor_mixed_establish.h"
#include "editor_mixed_checked.h"
#include "editor_mixed_bridges.h"

enum pt_editor_mixed_session_result {
    PT_EDITOR_MIXED_SESSION_INVALID = 0,
    PT_EDITOR_MIXED_SESSION_PENDING,
    PT_EDITOR_MIXED_SESSION_READY,
    PT_EDITOR_MIXED_SESSION_CANCELLED,
    PT_EDITOR_MIXED_SESSION_ESTABLISH_ERROR,
    PT_EDITOR_MIXED_SESSION_BRIDGE_ERROR,
    PT_EDITOR_MIXED_SESSION_CHECKED_ERROR,
    PT_EDITOR_MIXED_SESSION_FAULT,
    PT_EDITOR_MIXED_SESSION_CLOSED
};
enum pt_editor_mixed_session_phase {
    PT_EDITOR_MIXED_SESSION_EMPTY = 0,
    PT_EDITOR_MIXED_SESSION_MASTERS,
    PT_EDITOR_MIXED_SESSION_HANDOFF,
    PT_EDITOR_MIXED_SESSION_CHECKED,
    PT_EDITOR_MIXED_SESSION_PREPARED,
    PT_EDITOR_MIXED_SESSION_FAILED,
    PT_EDITOR_MIXED_SESSION_FINISHED
};
struct pt_editor_mixed_prepare_session_inputs {
    struct pt_editor_mixed *binding;
    struct pt_editor_mixed_establish *establish;
    struct pt_editor_mixed_bridges *bridges;
    struct pt_mixed_established *checked;
    struct pt_editor_mixed_bridge_inputs devices;
    struct pt_render_options options;
    struct pt_paula_render_caps caps;
    struct pt_playback_format format;
    struct pt_sampler_storage_span contexts;
    unsigned work;
};
/* Opt-in, task-side, single-use preparation coordinator. Every control is
 * genuine readable ordinary storage; session/job/bridges/checked/editor/binding
 * are separate, initially zero-init where their own contracts require it. Keep
 * the original inputs and complete named storage alive/immutable until close
 * returns1. One ordinary composite contexts extent MUST contain the entire
 * session and all non-NULL sampler allocator/device/backend callback contexts.
 * Callback subobjects and their complete extents must lie in that composite;
 * starts are checked, residency/unknown pointee extents cannot be proved here.
 * The job, bridges, checked control, editor and binding are outside contexts.
 * No caller copying, direct field writes, underlying job/owner/engine calls or
 * transport adoption while admitted. Existing editor changes/disposal may use
 * the attached binding barrier and cancel this session.
 *
 * Begin guards the complete session and all subsequent controls before its
 * first metadata allocation. One advance does one establishment step, OR the
 * confirmed-stop + metadata-bind + checked-adoption handoff without yielding,
 * OR one checked prepare call. work1..4096 is an existing item/byte bound, not
 * wall-clock/stack/placement certification. Handoff and descriptor guards are
 * finite metadata work. READY is only checked software preparation; no cache
 * upload/device completion, scheduled activation or capacity certificate.
 * Preparation keeps completed 8/16/24 masters sampler-owned and authoritative.
 *
 * Failure/cancellation latches; advance cannot restart. Call close explicitly.
 * The private finish hook retains both idle bound engines even when checked
 * admission refuses; confirmed owner close precedes both engine closes and
 * zero-child finish. Pending drains/child/reentry retain every remaining hook
 * and context. close never traverses former source arrays, forces release,
 * allocates, starts voices, opens/services timers or changes the music schedule.
 * No UI/event-loop/PLAY wiring, native output, IRQ, DMA, timing/audio acceptance.
 */
struct pt_editor_mixed_prepare_session {
    const struct pt_editor_mixed_prepare_session_inputs *inputs;
    struct pt_editor_mixed_prepare_session_inputs saved;
    struct pt_sampler_storage_span parents[6];
    int (*checked_finish)(void *);
    void *checked_finish_context;
    struct pt_mixed_owner *captured;
    uint32_t revision,generation;
    unsigned phase,busy,close_call,finishing,bridges_bound;
    enum pt_editor_mixed_session_result result,first_error;
    enum pt_establish_result establish_result;
    enum pt_mixed_owner_result checked_result;
};
enum pt_editor_mixed_session_result pt_editor_mixed_prepare_session_begin(
    struct pt_editor_mixed_prepare_session *,const struct pt_editor_mixed_prepare_session_inputs *);
enum pt_editor_mixed_session_result pt_editor_mixed_prepare_session_advance(
    struct pt_editor_mixed_prepare_session *);
/* Metadata/hook status only; no backend predicate, source-array scan or output.
 * Observable reentry faults the session. This is not a new ready certificate. */
enum pt_editor_mixed_session_result pt_editor_mixed_prepare_session_get(
    struct pt_editor_mixed_prepare_session *);
/* One explicit attempt; success confirms all this session's ownership closed.
 * The attached editor binding remains attached. Original failure is retained
 * in first_error even after phase FINISHED; this control is never reused. */
int pt_editor_mixed_prepare_session_close(struct pt_editor_mixed_prepare_session *);
#endif
