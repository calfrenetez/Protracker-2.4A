#include "sampler_prepare_project.h"
#include "../core/render_storage_internal.h"
#include <stddef.h>
#include <string.h>
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return span(a,an)&&span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);
}
static int current(const struct pt_sampler_prepare_project *m)
{
    const struct pt_project *p=m->project;
    size_t k=offsetof(struct pt_project,channels)+offsetof(struct pt_channels,selected);
    return p->channels.selected<p->channels.count&&
        !memcmp(p,&m->snapshot,k)&&
        !memcmp((const unsigned char *)p+k+sizeof(p->channels.selected),
            (const unsigned char *)&m->snapshot+k+sizeof(p->channels.selected),
            sizeof(*p)-k-sizeof(p->channels.selected));
}
static int fresh(const struct pt_sampler_prepare_project *m,const void *p,size_t bytes)
{
    return apart(p,bytes,m->project,sizeof(*m->project))&&
        pt_render_project_storage_output_disjoint(&m->snapshot,p,bytes)&&
        pt_render_project_storage_output_disjoint(m->project,p,bytes);
}
static void *project_allocate(void *context,size_t bytes)
{
    struct pt_sampler_prepare_project *m=context;void *p;
    if(!current(m)){m->memory.faulted=1;return NULL;}
    p=m->base.allocate(m->base.context,bytes);
    /* Alias refusal precedes sampler child bookkeeping and stale cleanup. */
    if(p&&!fresh(m,p,bytes)){m->memory.faulted=1;return NULL;}
    if(!current(m))m->memory.faulted=1;
    /* Original sampler guard classifies its sources/parents, then disposes a
     * safe fresh return after any fault. It alone records child ownership. */
    return p;
}
static void project_release(void *context,void *p)
{
    struct pt_sampler_prepare_project *m=context;
    m->base.release(m->base.context,p);
}
int pt_sampler_prepare_project_begin(struct pt_sampler_prepare_project *m,
    const struct pt_sampler *s,const struct pt_project *p,const struct pt_allocator *a,
    const struct pt_sampler_storage_span *parents,unsigned count)
{
    struct pt_sampler_storage_span saved[PT_SAMPLER_PREPARE_PARENTS];
    struct pt_allocator base,adapter;unsigned i;
    if(!m||!s||!a||!span(m,sizeof(*m))||!span(a,sizeof(*a))||
       !a->allocate||!a->release||count>=PT_SAMPLER_PREPARE_PARENTS||
       !span(parents,count*sizeof(*parents))||!apart(m,sizeof(*m),a,sizeof(*a))||
       !apart(m,sizeof(*m),parents,count*sizeof(*parents))||
       !pt_sampler_output_disjoint(s,m,sizeof(*m))||
       !pt_render_project_storage_output_disjoint(p,m,sizeof(*m)))return 0;
    for(i=0;i<count;++i){if(!span(parents[i].data,parents[i].bytes))return 0;saved[i]=parents[i];}
    saved[count]=(struct pt_sampler_storage_span){m,sizeof(*m)};
    base=*a;adapter=(struct pt_allocator){m,project_allocate,project_release};
    if(!pt_sampler_prepare_memory_begin(&m->memory,s,&adapter,saved,count+1))return 0;
    m->base=base;m->project=p;memcpy(&m->snapshot,p,sizeof(*p));return 1;
}
int pt_sampler_prepare_project_finish(struct pt_sampler_prepare_project *m)
{return m&&pt_sampler_prepare_memory_finish(&m->memory);}
