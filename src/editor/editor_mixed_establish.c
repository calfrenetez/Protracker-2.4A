#include "editor_mixed_establish.h"
#include "editor_mixed_internal.h"
#include "../core/render_storage_internal.h"
#include <string.h>
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,an)&&span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);}
static int close_preparation(void *context)
{
    struct pt_editor_mixed_establish *c=context;
    if(!pt_sampler_establish_cancel(&c->job))return 0;
    c->binding=NULL;return 1;
}
enum pt_establish_result pt_editor_mixed_establish_begin(struct pt_editor_mixed_establish *c,
    struct pt_editor_mixed *o,const struct pt_sampler_storage_span *extra,unsigned count)
{
    struct pt_sampler_storage_span saved[PT_EDITOR_ESTABLISH_EXTRA];struct pt_editor *e;unsigned i;
    if(!c||!pt_editor_mixed_attached(o)||count>PT_EDITOR_ESTABLISH_EXTRA||
       !span(extra,count*sizeof(*extra))||!apart(c,sizeof(*c),o,sizeof(*o))||
       !apart(c,sizeof(*c),extra,count*sizeof(*extra)))return PT_ESTABLISH_INVALID;
    e=o->editor;
    if(c->binding||c->job.active||c->job.busy||o->owner||o->transport||o->preparation_close||o->preparation_context||
       !apart(c,sizeof(*c),e,sizeof(*e))||!apart(o,sizeof(*o),e,sizeof(*e))||
       !pt_sampler_output_disjoint(&e->sampler,c,sizeof(*c))||
       !pt_sampler_output_disjoint(&e->sampler,o,sizeof(*o))||
       !pt_render_project_storage_output_disjoint(e->project,c,sizeof(*c))||
       !pt_render_project_storage_output_disjoint(e->project,o,sizeof(*o)))return PT_ESTABLISH_INVALID;
    for(i=0;i<count;++i){if(!span(extra[i].data,extra[i].bytes))return PT_ESTABLISH_INVALID;saved[i]=extra[i];}
    memset(c,0,sizeof(*c));c->binding=o;
    c->parents[0]=(struct pt_sampler_storage_span){c,sizeof(*c)};
    c->parents[1]=(struct pt_sampler_storage_span){o,sizeof(*o)};
    c->parents[2]=(struct pt_sampler_storage_span){e,sizeof(*e)};
    c->parents[3]=(struct pt_sampler_storage_span){extra,count*sizeof(*extra)};
    for(i=0;i<count;++i)c->parents[4+i]=saved[i];
    o->preparation_context=c;o->preparation_close=close_preparation;
    return pt_sampler_establish_begin(&c->job,&e->sampler,e->project,&e->sampler.allocator,
        c->parents,4+count,e->history.revision);
}
enum pt_establish_result pt_editor_mixed_establish_step(struct pt_editor_mixed_establish *c,unsigned work)
{
    struct pt_editor_mixed *o;
    if(!c||!(o=c->binding)||!pt_editor_mixed_attached(o)||o->preparation_close!=close_preparation||
       o->preparation_context!=c||o->owner||o->transport)return PT_ESTABLISH_INVALID;
    return pt_sampler_establish_step(&c->job,o->editor->history.revision,o->editor->sampler.generation,work);
}
