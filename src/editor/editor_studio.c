#include "editor_studio.h"
static void close_song(struct pt_editor_studio *o) {pt_sampler_song_close(o->song);o->song=NULL;pt_sampler_invert_song_close(o->invert_song);o->invert_song=NULL;}
void pt_editor_studio_stop(struct pt_editor_studio *owner)
{if(owner) {void (*stop)(void *)=owner->output_stop;void *context=owner->output_context;owner->output_stop=NULL;owner->output_context=NULL;if(owner->queue) {pt_studio_queue_abort(owner->queue);owner->queue=NULL;owner->pump.held=0;owner->pump.ended=1;}close_song(owner);if(stop)stop(context);}}
static void stop_guard(void *context) {pt_editor_studio_stop(context);}
static int attached(const struct pt_editor_studio *o)
{return o && o->editor && o->editor->before_change==stop_guard && o->editor->before_change_context==o;}
int pt_editor_studio_attach(struct pt_editor_studio *o,struct pt_editor *e)
{
    if(!o || !e || o->editor || o->song || o->invert_song || o->queue || e->before_change)return 0;
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
enum pt_render_result pt_editor_studio_start_invert(struct pt_editor_studio *o,const struct pt_render_options *options,size_t budget)
{
    if(!attached(o))return PT_RENDER_INVALID;
    pt_editor_studio_stop(o);
    return pt_sampler_invert_song_open(&o->editor->sampler,o->editor->project,options,budget,&o->editor->sampler.allocator,&o->invert_song);
}
static enum pt_render_result pull_song(struct pt_editor_studio *o,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{
    if(o->invert_song)return pt_sampler_invert_song_pull(o->invert_song,frames,pcm,done);
    if(o->song)return pt_sampler_song_pull(o->song,frames,pcm,done);
    *pcm=NULL;*done=1;return PT_RENDER_OK;
}
enum pt_render_result pt_editor_studio_pull(struct pt_editor_studio *o,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{
    enum pt_render_result result;
    if(!pcm || !done || !frames || frames>256)return PT_RENDER_INVALID;
    *pcm=NULL;*done=1;
    if(!attached(o)) {pt_editor_studio_stop(o);return PT_RENDER_INVALID;}
    if(o->queue)return PT_RENDER_INVALID;
    result=pull_song(o,frames,pcm,done);
    if(result!=PT_RENDER_OK || *done)pt_editor_studio_stop(o);
    return result;
}

static enum pt_render_result producer_pull(void *context,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{
    struct pt_editor_studio *o=context;
    return pull_song(o,frames,pcm,done);
}
static void producer_stop(void *context) {close_song(context);}
static enum pt_render_result start_queued(struct pt_editor_studio *o,const struct pt_render_options *options,struct pt_studio_queue *queue,unsigned invert,size_t budget)
{
    struct pt_studio_producer source;enum pt_render_result result;
    if(!queue || (o && queue==o->queue))return PT_RENDER_INVALID;
    result=invert?pt_editor_studio_start_invert(o,options,budget):pt_editor_studio_start(o,options);if(result!=PT_RENDER_OK)return result;
    source=(struct pt_studio_producer){o,producer_pull,producer_stop};
    if(!pt_studio_pump_init(&o->pump,&source,queue)) {pt_editor_studio_stop(o);return PT_RENDER_INVALID;}
    o->queue=queue;return PT_RENDER_OK;
}
enum pt_render_result pt_editor_studio_start_queued(struct pt_editor_studio *o,const struct pt_render_options *options,struct pt_studio_queue *queue)
{return start_queued(o,options,queue,0,0);}
enum pt_render_result pt_editor_studio_start_invert_queued(struct pt_editor_studio *o,const struct pt_render_options *options,size_t budget,struct pt_studio_queue *queue)
{return start_queued(o,options,queue,1,budget);}
enum pt_pump_result pt_editor_studio_step(struct pt_editor_studio *o,unsigned frames)
{
    enum pt_pump_result result;
    if(!attached(o)) {pt_editor_studio_stop(o);return PT_PUMP_ERROR;}
    if(!o->queue)return PT_PUMP_FINISHED;
    result=pt_studio_pump_step(&o->pump,frames);
    if(result==PT_PUMP_ERROR && o->pump.error)pt_editor_studio_stop(o);
    return result;
}

int pt_editor_studio_bind_output_stop(struct pt_editor_studio *o,void (*stop)(void *),void *context)
{
    if(!attached(o) || !o->queue || !stop || o->output_stop)return 0;
    o->output_stop=stop;o->output_context=context;return 1;
}
