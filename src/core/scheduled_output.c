#include "scheduled_output.h"
#include "document.h"
#include <string.h>
struct entry {
    struct pt_scheduled_event event;struct pt_scheduled_owner owner;
    struct pt_scheduled_span spans[PT_SCHEDULED_SPANS];unsigned state;
};
struct pt_scheduled_output {
    struct pt_allocator allocator;struct pt_scheduled_grid grid;
    struct pt_scheduled_backend backend;struct pt_elapsed_clock clock;
    uint64_t serial,last_frame,last_ticks;unsigned ordered,seen,capacity,held,closing,failed,busy;
    struct entry entry[PT_SCHEDULED_BATCHES];
};
enum {QUEUED=1,SUBMITTED};
static int valid_span(const void *p,size_t n)
{return !n || (p && (uintptr_t)p<=UINTPTR_MAX-(n-1));}
static int disjoint(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return valid_span(a,an) && valid_span(b,bn) && (!an || !bn ||
        (x<=y?an<=y-x:bn<=x-y));
}
static int caps(const struct pt_scheduled_backend *b,unsigned n)
{
    return b && b->caps.flags==PT_SCHEDULED_REQUIRED && n && n<=PT_SCHEDULED_BATCHES &&
        b->caps.maximum_batches>=n && b->caps.maximum_batches<=PT_SCHEDULED_BATCHES &&
        b->caps.maximum_actions && b->caps.maximum_actions<=PT_SCHEDULED_ACTIONS &&
        b->read_clock && b->submit && b->poll && b->cancel;
}
enum pt_scheduled_result pt_scheduled_output_open(const struct pt_allocator *a,
    const struct pt_scheduled_grid *g,const struct pt_scheduled_backend *b,unsigned n,
    struct pt_scheduled_output **out)
{
    struct pt_scheduled_output *q;struct pt_elapsed_clock clock;
    if(!a || !a->allocate || !a->release || !g || !out || !g->generation ||
       g->frequency<g->rate || pt_elapsed_clock_init(&clock,g->frequency,g->rate,g->epoch,0)!=PT_ELAPSED_OK)
        return PT_SCHEDULED_INVALID;
    if(!caps(b,n))return PT_SCHEDULED_UNSUPPORTED;
    if(!b->context_bytes || !valid_span(b->context,b->context_bytes))return PT_SCHEDULED_INVALID;
    if(!disjoint(out,sizeof(*out),a,sizeof(*a)) || !disjoint(out,sizeof(*out),g,sizeof(*g)) ||
       !disjoint(out,sizeof(*out),b,sizeof(*b)) || !disjoint(out,sizeof(*out),b->context,b->context_bytes))return PT_SCHEDULED_INVALID;
    q=a->allocate(a->context,sizeof(*q));if(!q)return PT_SCHEDULED_CAPACITY;
    if(!disjoint(q,sizeof(*q),out,sizeof(*out)) || !disjoint(q,sizeof(*q),a,sizeof(*a)) ||
       !disjoint(q,sizeof(*q),g,sizeof(*g)) || !disjoint(q,sizeof(*q),b,sizeof(*b)) ||
       !disjoint(q,sizeof(*q),b->context,b->context_bytes)) {
        a->release(a->context,q);return PT_SCHEDULED_INVALID;
    }
    memset(q,0,sizeof(*q));q->allocator=*a;q->grid=*g;q->backend=*b;q->clock=clock;q->capacity=n;
    *out=q;return PT_SCHEDULED_OK;
}
static int input(const struct pt_scheduled_output *q,const struct pt_scheduled_batch *b,
    const struct pt_scheduled_owner *o,const uint64_t *out)
{
    unsigned i,j;size_t n;
    if(!b || !o || !out || !o->current || !o->release || !o->token || !o->spans ||
       !o->count || o->count>PT_SCHEDULED_SPANS || !b->count || b->count>q->backend.caps.maximum_actions ||
       b->generation!=q->grid.generation || !disjoint(b,sizeof(*b),q,sizeof(*q)) ||
       !disjoint(o,sizeof(*o),q,sizeof(*q)) || !disjoint(o->spans,o->count*sizeof(*o->spans),q,sizeof(*q)) ||
       !disjoint(out,sizeof(*out),q,sizeof(*q)) || !disjoint(out,sizeof(*out),b,sizeof(*b)) ||
       !disjoint(out,sizeof(*out),o,sizeof(*o)) || !disjoint(out,sizeof(*out),o->spans,o->count*sizeof(*o->spans)))return 0;
    if(!disjoint(out,sizeof(*out),q->backend.context,q->backend.context_bytes))return 0;
    for(i=0;i<q->capacity;++i)if(q->entry[i].state)
        for(j=0;j<q->entry[i].owner.count;++j)if(!disjoint(out,sizeof(*out),
            q->entry[i].spans[j].data,q->entry[i].spans[j].bytes))return 0;
    for(i=0;i<o->count;++i)if(!valid_span(o->spans[i].data,o->spans[i].bytes) ||
       !disjoint(out,sizeof(*out),o->spans[i].data,o->spans[i].bytes) ||
       !disjoint(q,sizeof(*q),o->spans[i].data,o->spans[i].bytes))return 0;
    for(i=0;i<b->count;++i) {
        const struct pt_scheduled_action *a=b->action+i;
        if(a->slot>=4 || a->volume>64)return 0;
        if(a->kind==PT_SCHEDULED_STOP) {if(a->data || a->words || a->period || a->volume)return 0;}
        else if(a->kind==PT_SCHEDULED_CONTROL) {if(a->data || a->words || !a->period)return 0;}
        else if(a->kind==PT_SCHEDULED_TRIGGER) {
            n=(size_t)a->words*2;
            if(!a->words || !a->period || ((uintptr_t)a->data&1) || !valid_span(a->data,n))return 0;
            for(j=0;j<o->count;++j) {
                uintptr_t x=(uintptr_t)a->data,y=(uintptr_t)o->spans[j].data;
                if(x>=y && x-y<=o->spans[j].bytes && n<=o->spans[j].bytes-(x-y))break;
            }
            if(j==o->count)return 0;
        }else return 0;
    }
    return 1;
}
static int window(const struct pt_scheduled_output *q,uint64_t frame,uint64_t *first,uint64_t *last)
{
    return frame!=UINT64_MAX &&
        pt_elapsed_clock_deadline(&q->clock,frame,first)==PT_ELAPSED_OK &&
        pt_elapsed_clock_deadline(&q->clock,frame+1,last)==PT_ELAPSED_OK && *first<*last;
}
enum pt_scheduled_result pt_scheduled_output_enqueue(struct pt_scheduled_output *q,
    const struct pt_scheduled_batch *b,const struct pt_scheduled_owner *o,uint64_t *out)
{
    struct entry *e;unsigned i;uint64_t first,last;
    if(!q || q->busy || q->closing || q->failed || !input(q,b,o,out))return PT_SCHEDULED_INVALID;
    if(q->ordered && b->frame<=q->last_frame)return PT_SCHEDULED_INVALID;
    if(!window(q,b->frame,&first,&last))return PT_SCHEDULED_CLOCK;
    if(q->held==q->capacity || q->serial==UINT64_MAX)return PT_SCHEDULED_CAPACITY;
    for(i=0;i<q->capacity;++i)if(q->entry[i].state &&
       q->entry[i].owner.context==o->context && q->entry[i].owner.token==o->token)return PT_SCHEDULED_INVALID;
    q->busy=1;i=o->current(o->context,o->token,b->generation)==1;q->busy=0;
    if(!i)return PT_SCHEDULED_STALE;
    for(i=0;i<q->capacity;++i)if(!q->entry[i].state)break;
    e=q->entry+i;memset(e,0,sizeof(*e));e->event.batch=*b;e->event.first=first;e->event.last=last;
    e->event.ticket=++q->serial;e->owner=*o;memcpy(e->spans,o->spans,o->count*sizeof(*o->spans));
    e->owner.spans=e->spans;e->state=QUEUED;++q->held;q->last_frame=b->frame;q->ordered=1;
    *out=e->event.ticket;return PT_SCHEDULED_OK;
}
static struct entry *find(struct pt_scheduled_output *q,uint64_t ticket)
{
    unsigned i;for(i=0;i<q->capacity;++i)if(q->entry[i].state && q->entry[i].event.ticket==ticket)return q->entry+i;
    return NULL;
}
enum pt_scheduled_result pt_scheduled_output_publish(struct pt_scheduled_output *q,uint64_t ticket)
{
    struct entry *e;uint64_t now;uint32_t frequency;int result;unsigned i;
    if(!q || q->busy || q->closing || q->failed || !(e=find(q,ticket)) || e->state!=QUEUED)return PT_SCHEDULED_INVALID;
    for(i=0;i<q->capacity;++i)if(q->entry[i].state==QUEUED &&
       q->entry[i].event.batch.frame<e->event.batch.frame)return PT_SCHEDULED_INVALID;
    q->busy=1;result=e->owner.current(e->owner.context,e->owner.token,e->event.batch.generation);q->busy=0;
    if(result!=1)return PT_SCHEDULED_STALE;
    q->busy=1;result=q->backend.read_clock(q->backend.context,&now,&frequency);q->busy=0;
    if(result!=1 || frequency!=q->grid.frequency || now<q->grid.epoch || (q->seen && now<q->last_ticks))return PT_SCHEDULED_CLOCK;
    if(now>=e->event.first)return PT_SCHEDULED_LATE;
    q->busy=1;result=q->backend.submit(q->backend.context,&e->event);q->busy=0;
    if(result==0)return PT_SCHEDULED_PENDING;
    e->state=SUBMITTED;q->last_ticks=now;q->seen=1;
    if(result==1)return PT_SCHEDULED_OK;
    q->failed=1;return PT_SCHEDULED_BACKEND;
}
static void release(struct pt_scheduled_output *q,struct entry *e)
{
    q->busy=1;e->owner.release(e->owner.context,e->owner.token);q->busy=0;
    memset(e,0,sizeof(*e));--q->held;
}
enum pt_scheduled_result pt_scheduled_output_poll(struct pt_scheduled_output *q,uint64_t ticket)
{
    struct entry *e;struct pt_scheduled_receipt r;int result,good;
    if(!q || q->busy || !(e=find(q,ticket)) || e->state!=SUBMITTED)return PT_SCHEDULED_INVALID;
    memset(&r,0,sizeof(r));q->busy=1;result=q->backend.poll(q->backend.context,ticket,&r);q->busy=0;
    if(result==0)return PT_SCHEDULED_PENDING;
    if(result!=1){q->failed=1;return PT_SCHEDULED_BACKEND;}
    good=r.completion==PT_SCHEDULED_CANCELLED || (r.completion==PT_SCHEDULED_EXECUTED &&
        r.observed>=e->event.first && r.observed<=r.issued && r.issued<e->event.last);
    release(q,e);if(!good){q->failed=1;return PT_SCHEDULED_BACKEND;}
    return PT_SCHEDULED_OK;
}
enum pt_scheduled_result pt_scheduled_output_stop(struct pt_scheduled_output *q)
{
    unsigned i;int result,failed=0;
    if(!q || q->busy)return PT_SCHEDULED_INVALID;
    q->closing=1;
    for(i=0;i<q->capacity;++i)if(q->entry[i].state) {
        struct entry *e=q->entry+i;
        if(e->state==QUEUED){release(q,e);continue;}
        q->busy=1;result=q->backend.cancel(q->backend.context,e->event.ticket);q->busy=0;
        if(result==1)release(q,e);
        else if(result!=0){q->failed=1;failed=1;}
    }
    return failed?PT_SCHEDULED_BACKEND:q->held?PT_SCHEDULED_PENDING:PT_SCHEDULED_OK;
}
unsigned pt_scheduled_output_held(const struct pt_scheduled_output *q)
{return q?q->held:0;}
int pt_scheduled_output_close(struct pt_scheduled_output *q)
{
    struct pt_allocator a;
    if(!q || q->held || q->busy)return 0;
    a=q->allocator;a.release(a.context,q);return 1;
}
