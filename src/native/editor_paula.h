#ifndef PT_NATIVE_EDITOR_PAULA_H
#define PT_NATIVE_EDITOR_PAULA_H
#include "paula_engine.h"
#include "../editor/editor_paula.h"
/* Frontend-owned, zero-init/noncopyable, serialized. Attach before any native
 * preparation/output; editor stop/change/dispose owns engine cleanup even before
 * song publication. Native storage outlives detach. Begin/advance do not start
 * voices: caller uses the attached prepared song and honest monotonic timing.
 * No PLAY/UI/event loop/timer installed here. No fallback for refused geometry. */
struct pt_native_editor_paula {
    struct pt_editor_paula binding;struct pt_native_paula_engine engine;
    struct pt_render_options options;unsigned active,begun,failed;
    /* Optional enclosing owner's final cleanup, fixed before attach and kept
     * alive/immutable through detach. No reader client survives song close. */
    int (*release_tail)(void *);void *release_tail_context;
};
static inline int pt_native_editor_paula_release(void *context)
{
    struct pt_native_editor_paula *n=context;
    if(!pt_native_paula_engine_close(&n->engine))return 0;
    if(n->release_tail && n->release_tail(n->release_tail_context)!=1)return 0;
    n->active=n->begun=n->failed=0;n->options=(struct pt_render_options){0};return 1;
}
static inline int pt_native_editor_paula_attach(struct pt_native_editor_paula *n,struct pt_editor *e)
{
    if(!n || n->active || n->engine.sampler || !pt_editor_paula_attach(&n->binding,e))return 0;
    if(!pt_editor_paula_bind_release(&n->binding,pt_native_editor_paula_release,n)) {
        pt_editor_paula_detach(&n->binding);return 0;
    }
    return 1;
}
static inline enum pt_paula_song_result pt_native_editor_paula_begin(
    struct pt_native_editor_paula *n,const struct pt_render_options *options,size_t chip_budget)
{
    if(!n || !n->binding.editor || n->binding.release!=pt_native_editor_paula_release ||
       n->binding.release_context!=n || n->binding.release_pending || n->active || !options)
        return PT_PAULA_SONG_INVALID;
    n->options=*options;n->active=1;
    if(!pt_native_paula_engine_begin(&n->engine,&n->binding.editor->sampler,
        n->binding.editor->project,chip_budget)){n->failed=1;return PT_PAULA_SONG_DEVICE;}
    return PT_PAULA_SONG_PREPARING;
}
/* One reservation/bind/begin OR one bounded analysis/master preparation call, no waiting/output.
 * Failure is terminal until editor stop confirms all cleanup, including cases
 * with no song published. Options/caps immutable for each run. */
static inline enum pt_paula_song_result pt_native_editor_paula_advance(
    struct pt_native_editor_paula *n,struct pt_paula_preflight_report *report)
{
    int r;enum pt_paula_song_result result;
    if(!n || !n->active || n->failed || n->binding.release_pending)return PT_PAULA_SONG_INVALID;
    if(!n->begun) {
        r=pt_native_paula_engine_advance(&n->engine);
        if(r<0){n->failed=1;return PT_PAULA_SONG_DEVICE;}if(!r)return PT_PAULA_SONG_PREPARING;
        result=pt_editor_paula_begin(&n->binding,&n->engine.voices,&n->options,&n->engine.output.caps);
        if(result!=PT_PAULA_SONG_PREPARING){n->failed=1;return result;}
        n->begun=1;return PT_PAULA_SONG_PREPARING;
    }
    result=pt_editor_paula_prepare(&n->binding,report);
    if(result!=PT_PAULA_SONG_OK && result!=PT_PAULA_SONG_PREPARING)n->failed=1;
    return result;
}
#endif
