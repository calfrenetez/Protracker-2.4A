#ifndef PT_EDITOR_MIXED_H
#define PT_EDITOR_MIXED_H
#include "editor.h"
#include "mixed_transport.h"
/* Zero-init, noncopyable, serialized editor-thread binding. Attach refuses any
 * existing change guard. Editor, binding, engines, pump and timer contexts stay
 * alive until stop confirms release; detach before editor reinit/free, including
 * after successful dispose. No UI action or output/device binding installed. */
struct pt_editor_mixed {
    struct pt_editor *editor;struct pt_mixed_owner *owner;
    struct pt_mixed_transport *transport;
    /* Private opt-in preparation adapter. Never set/copy/edit these directly.
     * Kept through refused cancellation; no new dependency in legacy binding. */
    int (*preparation_close)(void *);void *preparation_context;
};
int pt_editor_mixed_attach(struct pt_editor_mixed *,struct pt_editor *);
/* One bounded close attempt. Uncertain alarm/readers/counter retain the binding
 * and veto editor changes. Cancels adopted pre-borrow preparation first; completed
 * masters stay sampler-owned. Explicit later close required; never force release. */
int pt_editor_mixed_stop(struct pt_editor_mixed *);
int pt_editor_mixed_detach(struct pt_editor_mixed *);
/* Both idle bound engines must use THIS editor's sampler/project. Begin uses its
 * sampler allocator; preparation never starts output. Engines/cache contexts
 * must be rebound after confirmed close before starting a fresh owner. */
enum pt_mixed_owner_result pt_editor_mixed_begin(struct pt_editor_mixed *,
    struct pt_paula_voices *,struct pt_wavetable_voices *,const struct pt_render_options *,
    const struct pt_paula_render_caps *,const struct pt_playback_format *);
enum pt_mixed_owner_result pt_editor_mixed_prepare(struct pt_editor_mixed *,struct pt_mixed_report *);
/* Adopts a zero-init caller-owned pump and already-open private timer API only
 * when transport begin adopts resources (even if clocked begin fails). Otherwise
 * caller still owns timer cleanup. No external pump/owner calls while attached. */
enum pt_mixed_owner_result pt_editor_mixed_start(struct pt_editor_mixed *,struct pt_mixed_transport *,
    uint64_t delay,uint32_t quantum,const struct pt_mixed_timer_api *);
enum pt_mixed_owner_result pt_editor_mixed_service(struct pt_editor_mixed *);
uint32_t pt_editor_mixed_signal(const struct pt_editor_mixed *);
#endif
