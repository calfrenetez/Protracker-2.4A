#include "sampler_prepare_memory.h"
#include <string.h>
static int span(const void *p,size_t n)
{return !n || (p && n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return span(a,an)&&span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);
}
static int current(const struct pt_sampler_prepare_memory *m)
{return !memcmp(m->sampler,&m->snapshot,sizeof(m->snapshot));}
static int fresh(const struct pt_sampler_prepare_memory *m,const void *p,size_t bytes)
{
    unsigned i;
    if(!p || !bytes || !apart(p,bytes,m,sizeof(*m)) ||
       !apart(p,bytes,m->sampler,sizeof(*m->sampler)) ||
       !pt_sampler_output_disjoint(&m->snapshot,p,bytes) ||
       !pt_sampler_output_disjoint(m->sampler,p,bytes))return 0;
    for(i=0;i<m->parent_count;++i)
        if(!apart(p,bytes,m->parents[i].data,m->parents[i].bytes))return 0;
    for(i=0;i<PT_SAMPLER_PREPARE_BLOCKS;++i)
        if(!apart(p,bytes,m->block[i].data,m->block[i].bytes))return 0;
    return 1;
}
static void *prepare_allocate(void *context,size_t bytes)
{
    struct pt_sampler_prepare_memory *m=context;void *p;unsigned i;
    if(!m || !m->active)return NULL;
    if(m->busy){m->faulted=1;return NULL;}
    if(m->faulted || !bytes)return NULL;
    if(!current(m)){m->faulted=1;return NULL;}
    for(i=0;i<PT_SAMPLER_PREPARE_BLOCKS && m->block[i].data;++i){}
    if(i==PT_SAMPLER_PREPARE_BLOCKS)return NULL;
    m->busy=1;p=m->base.allocate(m->base.context,bytes);
    /* Classify known aliases before cleanup or any initialization. */
    if(p && !fresh(m,p,bytes)){m->faulted=1;m->busy=0;return NULL;}
    if(!current(m))m->faulted=1;
    if(m->faulted){if(p)m->base.release(m->base.context,p);m->busy=0;return NULL;}
    if(p){m->block[i].data=p;m->block[i].bytes=bytes;}
    m->busy=0;return p;
}
static void prepare_release(void *context,void *p)
{
    struct pt_sampler_prepare_memory *m=context;unsigned i;
    if(!m || !m->active)return;
    if(m->busy){m->faulted=1;return;}
    if(!p){m->faulted=1;return;}
    for(i=0;i<PT_SAMPLER_PREPARE_BLOCKS && m->block[i].data!=p;++i){}
    if(i==PT_SAMPLER_PREPARE_BLOCKS){m->faulted=1;return;}
    /* Retire the exact owned slot before callback. Reentry cannot release twice. */
    m->block[i].data=NULL;m->block[i].bytes=0;m->busy=1;
    m->base.release(m->base.context,p);m->busy=0;
}
int pt_sampler_prepare_memory_begin(struct pt_sampler_prepare_memory *m,
    const struct pt_sampler *s,const struct pt_allocator *a,
    const struct pt_sampler_storage_span *parents,unsigned count)
{
    unsigned i;
    if(!m || !s || !a || !a->allocate || !a->release ||
       count>PT_SAMPLER_PREPARE_PARENTS || !span(parents,count*sizeof(*parents)) ||
       !apart(m,sizeof(*m),a,sizeof(*a)) ||
       !apart(m,sizeof(*m),parents,count*sizeof(*parents)) ||
       !pt_sampler_output_disjoint(s,m,sizeof(*m)) || m->active)return 0;
    for(i=0;i<count;++i)if(!span(parents[i].data,parents[i].bytes))return 0;
    memset(m,0,sizeof(*m));m->base=*a;m->sampler=s;
    memcpy(&m->snapshot,s,sizeof(m->snapshot));
    if(count)memcpy(m->parents,parents,count*sizeof(*parents));
    m->parent_count=count;m->active=1;
    m->allocator=(struct pt_allocator){m,prepare_allocate,prepare_release};return 1;
}
int pt_sampler_prepare_memory_finish(struct pt_sampler_prepare_memory *m)
{
    unsigned i;if(!m)return 0;
    if(m->busy){m->faulted=1;return 0;}
    if(!m->active)return 1;
    for(i=0;i<PT_SAMPLER_PREPARE_BLOCKS;++i)if(m->block[i].data)return 0;
    m->active=0;return 1;
}
