#include "sampler_lifetime_internal.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>
static int span(const void *p,size_t n)
{return !n || (p && n<=UINTPTR_MAX-(uintptr_t)p);}
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return !span(a,an)||!span(b,bn)||(an&&bn&&x<y+bn&&y<x+an);
}
static int live_links(const struct pt_sampler_live *n)
{
    const struct pt_sampler *s=n->owner;
    return s && s->live_count && ((!n->previous&&s->live_head==n) ||
        (n->previous&&n->previous->next==n&&n->previous->owner==s)) &&
        (!n->next || (n->next->previous==n&&n->next->owner==s));
}
static int pending_links(const struct pt_sampler_pending *n)
{
    const struct pt_sampler *s=n->owner;
    return s && s->pending_count && ((!n->previous&&s->pending_head==n) ||
        (n->previous&&n->previous->next==n&&n->previous->owner==s)) &&
        (!n->next || (n->next->previous==n&&n->next->owner==s));
}
int pt_sampler_live_add(struct pt_sampler *s,struct pt_sampler_live *n,unsigned kind,
    const void *a,size_t ab,const void *b,size_t bb,const void *c,size_t cb)
{
    struct pt_sampler_live_span spans[3];unsigned count=0;
    if(!s||!n||n->owner||!kind||kind>PT_LIVE_ESTABLISH||!ab||
       !span(a,ab)||!span(b,bb)||!span(c,cb)||s->lifetime_fault||
       s->live_count==SIZE_MAX||(!s->live_head != !s->live_count))return 0;
    if(s->live_head && (s->live_head->owner!=s||s->live_head->previous))return 0;
    spans[count++]=(struct pt_sampler_live_span){a,ab};
    if(bb)spans[count++]=(struct pt_sampler_live_span){b,bb};
    if(cb)spans[count++]=(struct pt_sampler_live_span){c,cb};
    memset(n,0,sizeof(*n));n->owner=s;n->kind=kind;n->count=count;
    memcpy(n->spans,spans,count*sizeof(*spans));n->next=s->live_head;
    if(n->next)n->next->previous=n;
    s->live_head=n;++s->live_count;return 1;
}
int pt_sampler_live_remove(struct pt_sampler_live *n)
{
    struct pt_sampler *s;
    if(!n)return 0;
    if(!n->owner)return 1;
    s=n->owner;
    if(!live_links(n)){s->lifetime_fault=1;return 0;}
    if(n->previous)n->previous->next=n->next;else s->live_head=n->next;
    if(n->next)n->next->previous=n->previous;
    --s->live_count;memset(n,0,sizeof(*n));return 1;
}
int pt_sampler_pending_add(struct pt_sampler *s,struct pt_sampler_pending *n,
    unsigned kind,const void *control,size_t bytes,const void *project,unsigned slot,unsigned generation)
{
    if(!s||!n||n->owner||!span(control,bytes)||!bytes||!kind||
       kind>PT_PENDING_CALL||s->lifetime_fault||s->pending_count==SIZE_MAX||
       (!s->pending_head != !s->pending_count))return 0;
    if(s->pending_head && (s->pending_head->owner!=s||s->pending_head->previous))return 0;
    memset(n,0,sizeof(*n));n->owner=s;n->kind=kind;n->control=control;n->bytes=bytes;
    n->project=project;n->slot=slot;n->generation=generation;n->next=s->pending_head;
    if(n->next)n->next->previous=n;
    s->pending_head=n;++s->pending_count;return 1;
}
int pt_sampler_pending_retire(struct pt_sampler_pending *n,const struct pt_sampler_live *live)
{
    unsigned i;
    if(!n||!live||n->kind!=PT_PENDING_CALL||!n->owner||n->owner!=live->owner||
       !pending_links(n)||!live_links(live)||!live->count||live->count>3||
       n->retiring_count)return 0;
    for(i=0;i<live->count;++i)if(!span(live->spans[i].data,live->spans[i].bytes))return 0;
    memcpy(n->retiring,live->spans,live->count*sizeof(*n->retiring));
    n->retiring_count=live->count;return 1;
}
int pt_sampler_pending_remove(struct pt_sampler_pending *n)
{
    struct pt_sampler *s;
    if(!n)return 0;
    if(!n->owner)return 1;
    s=n->owner;
    if(!pending_links(n)){s->lifetime_fault=1;return 0;}
    if(n->previous)n->previous->next=n->next;else s->pending_head=n->next;
    if(n->next)n->next->previous=n->previous;
    --s->pending_count;memset(n,0,sizeof(*n));return 1;
}
int pt_sampler_pending_transfer(struct pt_sampler_pending *old,struct pt_sampler_pending *next,
    const void *control,size_t bytes)
{
    struct pt_sampler *s;
    if(!old||!next||old==next||next->owner||!span(control,bytes)||!bytes)return 0;
    s=old->owner;
    if(!pending_links(old)){if(s)s->lifetime_fault=1;return 0;}
    memcpy(next,old,sizeof(*next));next->control=control;next->bytes=bytes;
    if(next->previous)next->previous->next=next;else s->pending_head=next;
    if(next->next)next->next->previous=next;
    memset(old,0,sizeof(*old));return 1;
}
int pt_sampler_lifetime_idle(const struct pt_sampler *s)
{return s&&!s->lifetime_fault&&!s->pending_head&&!s->pending_count;}
int pt_sampler_lifetime_output_disjoint(const struct pt_sampler *s,const void *out,size_t bytes)
{
    const struct pt_sampler_live *n,*previous=NULL;
    const struct pt_sampler_pending *pending,*before=NULL;size_t i;unsigned k;
    if(!s||!out||!bytes||!span(out,bytes)||s->lifetime_fault||
       s->live_count>PT_SAMPLER_LIVE_CHECK_LIMIT||
       s->pending_count>PT_SAMPLER_LIVE_CHECK_LIMIT-s->live_count||
       overlap(out,bytes,s,sizeof(*s))||(!s->live_head != !s->live_count)||
       (!s->pending_head != !s->pending_count))return 0;
    n=s->live_head;
    for(i=0;i<s->live_count;++i){
        if(!n||n->owner!=s||n->previous!=previous||!n->count||n->count>3||
           !n->kind||n->kind>PT_LIVE_ESTABLISH)return 0;
        for(k=0;k<n->count;++k)
            if(overlap(out,bytes,n->spans[k].data,n->spans[k].bytes))return 0;
        previous=n;n=n->next;
    }
    if(n)return 0;
    pending=s->pending_head;
    for(i=0;i<s->pending_count;++i){
        if(!pending||pending->owner!=s||pending->previous!=before||
           !pending->kind||pending->kind>PT_PENDING_CALL||
           !pending->bytes||pending->retiring_count>3||
           overlap(out,bytes,pending->control,pending->bytes))return 0;
        for(k=0;k<pending->retiring_count;++k)
            if(overlap(out,bytes,pending->retiring[k].data,pending->retiring[k].bytes))return 0;
        before=pending;pending=pending->next;
    }
    return !pending;
}

struct table_allocation {struct pt_sampler_live live;struct pt_sample samples[PT_PROJECT_SAMPLES];};
size_t pt_sampler_table_allocation_bytes(void){return sizeof(struct table_allocation);}
struct pt_sample *pt_sampler_table_allocate(struct pt_sampler *s)
{
    struct table_allocation *a;size_t n=sizeof(*a);struct pt_sampler_pending call={0};
    struct pt_allocator allocator;
    if(!s||s->bytes>s->budget||n>s->budget-s->bytes)return NULL;
    if(!pt_sampler_pending_add(s,&call,PT_PENDING_CALL,&call,sizeof(call),NULL,0,s->generation))return NULL;
    allocator=s->allocator;a=allocator.allocate(allocator.context,n);
    if(!a){pt_sampler_pending_remove(&call);return NULL;}
    /* Exact returned allocation, under the ordinary fresh allocator contract.
     * Cache its numeric extent before any release or another callback. */
    call.retiring[0]=(struct pt_sampler_live_span){a,n};call.retiring_count=1;
    if(s->bytes>s->budget||n>s->budget-s->bytes){
        allocator.release(allocator.context,a);pt_sampler_pending_remove(&call);return NULL;
    }
    memset(a,0,sizeof(*a));
    if(!pt_sampler_live_add(s,&a->live,PT_LIVE_TABLE,a,n,NULL,0,NULL,0)){
        allocator.release(allocator.context,a);pt_sampler_pending_remove(&call);return NULL;
    }
    s->bytes+=n;pt_sampler_pending_remove(&call);return a->samples;
}
void pt_sampler_table_release(struct pt_sampler *s,struct pt_sample *table)
{
    struct table_allocation *a;struct pt_sampler_pending call={0};
    if(!s||!table)return;
    a=(struct table_allocation *)((unsigned char *)table-
        offsetof(struct table_allocation,samples));
    if(a->live.owner!=s||a->live.kind!=PT_LIVE_TABLE){s->lifetime_fault=1;return;}
    if(!pt_sampler_pending_add(s,&call,PT_PENDING_CALL,&call,sizeof(call),NULL,0,s->generation))return;
    if(!pt_sampler_pending_retire(&call,&a->live)||!pt_sampler_live_remove(&a->live)){
        s->lifetime_fault=1;pt_sampler_pending_remove(&call);return;
    }
    s->bytes-=sizeof(*a);s->allocator.release(s->allocator.context,a);pt_sampler_pending_remove(&call);
}
