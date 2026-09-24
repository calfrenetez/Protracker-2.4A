#include "editor_studio.h"
static void close_song(struct pt_editor_studio *o) {pt_sampler_song_close(o->song);o->song=NULL;}
void pt_editor_studio_stop(struct pt_editor_studio *owner)
{if(owner) {if(owner->queue) {pt_studio_queue_abort(owner->queue);owner->queue=NULL;owner->pump.held=0;owner->pump.ended=1;}close_song(owner);}}
static void stop_guard(void *context) {pt_editor_studio_stop(context);}
static int attached(const struct pt_editor_studio *o)
{return o && o->editor && o->editor->before_change==stop_guard && o->editor->before_change_context==o;}
int pt_editor_studio_attach(struct pt_editor_studio *o,struct pt_editor *e)
{
    if(!o || !e || o->editor || o->song || o->queue || e->before_change)return 0;
    o->editor=e;pt_editor_change_guard(e,stop_guard,o);return 1;
}
void pt_editor_studio_detach(struct pt_editor_studio *o)
{
    if(!o)return;
    pt_editor_studio_stop(o);
    if(attached(o))pt_editor_change_guard(o->editor,NULL,NULL);
    o->editor=NULL;
}
enum pt_render_result pt_editor_studio_start(struct pt_editor_studio *o,const struct pt_render_options *options)
{
    if(!attached(o))return PT_RENDER_INVALID;
    pt_editor_studio_stop(o);
    return pt_sampler_song_open(&o->editor->sampler,o->editor->project,options,&o->editor->sampler.allocator,&o->song);
}
enum pt_render_result pt_editor_studio_pull(struct pt_editor_studio *o,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{
    enum pt_render_result result;
    if(!pcm || !done || !frames || frames>256)return PT_RENDER_INVALID;
    *pcm=NULL;*done=1;
    if(!attached(o)) {pt_editor_studio_stop(o);return PT_RENDER_INVALID;}
    if(o->queue)return PT_RENDER_INVALID;
    if(!o->song)return PT_RENDER_OK;
    result=pt_sampler_song_pull(o->song,frames,pcm,done);
    if(result!=PT_RENDER_OK || *done)pt_editor_studio_stop(o);
    return result;
}

static enum pt_render_result producer_pull(void *context,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{
    struct pt_editor_studio *o=context;
    if(!o->song) {*pcm=NULL;*done=1;return PT_RENDER_OK;}
    return pt_sampler_song_pull(o->song,frames,pcm,done);
}
static void producer_stop(void *context) {close_song(context);}
enum pt_render_result pt_editor_studio_start_queued(struct pt_editor_studio *o,const struct pt_render_options *options,struct pt_studio_queue *queue)
{
    struct pt_studio_producer source;enum pt_render_result result;
    if(!queue || (o && queue==o->queue))return PT_RENDER_INVALID;
    result=pt_editor_studio_start(o,options);if(result!=PT_RENDER_OK)return result;
    source=(struct pt_studio_producer){o,producer_pull,producer_stop};
    if(!pt_studio_pump_init(&o->pump,&source,queue)) {pt_editor_studio_stop(o);return PT_RENDER_INVALID;}
    o->queue=queue;return PT_RENDER_OK;
}
enum pt_pump_result pt_editor_studio_step(struct pt_editor_studio *o,unsigned frames)
{
    if(!attached(o)) {pt_editor_studio_stop(o);return PT_PUMP_ERROR;}
    if(!o->queue)return PT_PUMP_FINISHED;
    return pt_studio_pump_step(&o->pump,frames);
}
