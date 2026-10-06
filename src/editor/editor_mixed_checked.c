#include "editor_mixed_checked.h"
#include "editor_mixed_internal.h"
#include "mixed_owner_established_internal.h"
#include "../core/render_storage_internal.h"
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,an)&&span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);}
static int finish(void *context)
{return pt_mixed_owner_established_finish(context);}
enum pt_mixed_owner_result pt_editor_mixed_checked_begin(struct pt_editor_mixed *o,
    struct pt_mixed_established *c,struct pt_paula_voices *p,struct pt_wavetable_voices *w,
    const struct pt_render_options *options,const struct pt_paula_render_caps *caps,const struct pt_playback_format *format,
    struct pt_sampler_storage_span contexts,unsigned work)
{
    struct pt_editor *e;struct pt_sampler_storage_span parents[2];unsigned count=1,i;
    struct pt_sampler_storage_span controls[8];
    if(!pt_editor_mixed_attached(o)||!c||!p||!w||!p->bridge||!w->bridge||!w->bridge->backend||
       !w->bridge->backend->reservation||!options||!caps||!format||!work||work>4096||
       o->owner||o->transport||o->preparation_close||o->preparation_context||o->owner_finish||o->owner_finish_context||
       c->storage.memory.active||c->startup||c->starting||!span(contexts.data,contexts.bytes))return PT_MIXED_OWNER_INVALID;
    e=o->editor;
    if(p->bridge->sampler!=&e->sampler||w->bridge->sampler!=&e->sampler||
       p->bridge->project!=e->project||w->bridge->project!=e->project||
       !apart(o,sizeof(*o),c,sizeof(*c))||!apart(o,sizeof(*o),e,sizeof(*e))||
       !apart(c,sizeof(*c),e,sizeof(*e))||!apart(o,sizeof(*o),contexts.data,contexts.bytes)||
       !apart(c,sizeof(*c),contexts.data,contexts.bytes)||
       !pt_sampler_output_disjoint(&e->sampler,o,sizeof(*o))||!pt_sampler_output_disjoint(&e->sampler,c,sizeof(*c))||
       !pt_render_project_storage_output_disjoint(e->project,o,sizeof(*o))||
       !pt_render_project_storage_output_disjoint(e->project,c,sizeof(*c)))return PT_MIXED_OWNER_INVALID;
    controls[0]=(struct pt_sampler_storage_span){p,sizeof(*p)};controls[1]=(struct pt_sampler_storage_span){w,sizeof(*w)};
    controls[2]=(struct pt_sampler_storage_span){p->bridge,sizeof(*p->bridge)};
    controls[3]=(struct pt_sampler_storage_span){w->bridge,sizeof(*w->bridge)};
    controls[4]=(struct pt_sampler_storage_span){w->bridge->backend,sizeof(*w->bridge->backend)};
    controls[5]=(struct pt_sampler_storage_span){w->bridge->backend->reservation,sizeof(*w->bridge->backend->reservation)};
    controls[6]=(struct pt_sampler_storage_span){options,sizeof(*options)};
    controls[7]=(struct pt_sampler_storage_span){caps,sizeof(*caps)};
    for(i=0;i<8;++i)if(!apart(o,sizeof(*o),controls[i].data,controls[i].bytes))return PT_MIXED_OWNER_INVALID;
    if(!apart(o,sizeof(*o),format,sizeof(*format)))return PT_MIXED_OWNER_INVALID;
    parents[0]=(struct pt_sampler_storage_span){e,sizeof(*e)};
    if(contexts.bytes)parents[count++]=contexts;
    o->owner_finish_context=c;o->owner_finish=finish;
    return pt_mixed_owner_established_begin_bound(c,p,w,options,caps,format,&e->sampler.allocator,
        o,sizeof(*o),parents,count,e->history.revision,work,&o->owner);
}
