#include "editor_mixed.h"
int pt_editor_mixed_stop(struct pt_editor_mixed *o)
{
    if(!o)return 0;
    if(o->transport) {
        if(!pt_mixed_transport_close(o->transport))return 0;
        o->transport=NULL;
    }
    return pt_mixed_owner_close(&o->owner);
}
static int barrier(void *context){return pt_editor_mixed_stop(context);}
static int attached(const struct pt_editor_mixed *o)
{return o && o->editor && o->editor->change_ready==barrier && o->editor->before_change_context==o;}
int pt_editor_mixed_attach(struct pt_editor_mixed *o,struct pt_editor *e)
{
    if(!o || o->editor || o->owner || o->transport || !pt_editor_change_barrier(e,barrier,o))return 0;
    o->editor=e;return 1;
}
int pt_editor_mixed_detach(struct pt_editor_mixed *o)
{
    if(!o || !pt_editor_mixed_stop(o))return 0;
    if(attached(o) && !pt_editor_change_barrier(o->editor,NULL,NULL))return 0;
    o->editor=NULL;return 1;
}
enum pt_mixed_owner_result pt_editor_mixed_begin(struct pt_editor_mixed *o,
    struct pt_paula_voices *p,struct pt_wavetable_voices *w,const struct pt_render_options *options,
    const struct pt_paula_render_caps *caps,const struct pt_playback_format *format)
{
    if(!attached(o) || o->owner || o->transport || !p || !w || !p->bridge || !w->bridge ||
       p->bridge->sampler!=&o->editor->sampler || w->bridge->sampler!=&o->editor->sampler ||
       p->bridge->project!=o->editor->project || w->bridge->project!=o->editor->project)
        return PT_MIXED_OWNER_INVALID;
    return pt_mixed_owner_begin(p,w,options,caps,format,&o->editor->sampler.allocator,&o->owner);
}
enum pt_mixed_owner_result pt_editor_mixed_prepare(struct pt_editor_mixed *o,struct pt_mixed_report *out)
{return attached(o) && !o->transport?pt_mixed_owner_prepare(o->owner,out):PT_MIXED_OWNER_INVALID;}
enum pt_mixed_owner_result pt_editor_mixed_start(struct pt_editor_mixed *o,struct pt_mixed_transport *t,
    uint64_t delay,uint32_t quantum,const struct pt_mixed_timer_api *api)
{
    enum pt_mixed_owner_result r;
    if(!attached(o) || !o->owner || o->transport || !t || t->active)return PT_MIXED_OWNER_INVALID;
    r=pt_mixed_transport_begin(t,&o->owner,delay,quantum,api);
    if(t->active)o->transport=t;
    return r;
}
enum pt_mixed_owner_result pt_editor_mixed_service(struct pt_editor_mixed *o)
{return attached(o) && o->transport?pt_mixed_transport_service(o->transport):PT_MIXED_OWNER_INVALID;}
uint32_t pt_editor_mixed_signal(const struct pt_editor_mixed *o)
{return attached(o)?pt_mixed_transport_signal(o->transport):0;}
