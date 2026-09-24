#include "editor_studio.h"
void pt_editor_studio_stop(struct pt_editor_studio *owner)
{if(owner) {pt_sampler_song_close(owner->song);owner->song=NULL;}}
static void stop_guard(void *context) {pt_editor_studio_stop(context);}
static int attached(const struct pt_editor_studio *o)
{return o && o->editor && o->editor->before_change==stop_guard && o->editor->before_change_context==o;}
int pt_editor_studio_attach(struct pt_editor_studio *o,struct pt_editor *e)
{
    if(!o || !e || o->editor || o->song || e->before_change)return 0;
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
    if(!o->song)return PT_RENDER_OK;
    result=pt_sampler_song_pull(o->song,frames,pcm,done);
    if(result!=PT_RENDER_OK || *done)pt_editor_studio_stop(o);
    return result;
}
