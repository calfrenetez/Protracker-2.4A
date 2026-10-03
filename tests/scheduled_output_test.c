#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/core/scheduled_output.h"
#include "../src/core/document.h"
#include "../src/core/sample_cache.h"
struct pool {unsigned allocs,frees,fail;void *live;size_t bytes;};
static void *allocate(void *context,size_t n)
{struct pool *p=context;void *v;if(p->fail)return NULL;v=malloc(n);assert(v);++p->allocs;p->live=v;p->bytes=n;return v;}
static void deallocate(void *context,void *v,size_t n)
{struct pool *p=context;assert(v && n);++p->frees;if(p->live==v)p->live=NULL;free(v);}
static void queue_free(void *context,void *v)
{struct pool *p=context;deallocate(context,v,p->bytes);}
struct prepared {
    union {uint64_t scalar;int32_t values[32];} master;
    struct pt_sample_cache cache;struct pt_cache_lease lease;
    struct pool memory;unsigned held,released,checks,valid;
    uint64_t generation,token;
    struct pt_scheduled_span spans[3];struct pt_scheduled_owner owner;
};
static int current(void *context,uint64_t token,uint64_t generation)
{struct prepared *p=context;++p->checks;return p->held && p->valid && token==p->token && generation==p->generation &&
 p->cache.entry[p->lease.slot].valid==1 && p->cache.entry[p->lease.slot].version==generation && pt_cache_data(&p->cache,p->lease)!=NULL;}
static void retired(void *context,uint64_t token)
{struct prepared *p=context;assert(p->held && token==p->token);assert(pt_cache_unpin(&p->cache,p->lease));p->held=0;++p->released;}
static void prepare(struct prepared *p,uint64_t token)
{
    unsigned i;uint8_t *data;memset(p,0,sizeof(*p));p->generation=7;p->token=token;
    for(i=0;i<32;++i)p->master.values[i]=(int32_t)(0x123401U+i); /* includes precision and unused capacity */
    pt_cache_init(&p->cache,&p->memory,allocate,deallocate,128);
    assert(pt_cache_take(&p->cache,token,7,16,&p->lease)==PT_CACHE_LOAD);
    data=pt_cache_data(&p->cache,p->lease);for(i=0;i<16;++i)data[i]=(uint8_t)(i+1);
    assert(pt_cache_publish(&p->cache,p->lease));p->held=p->valid=1;
    p->spans[0]=(struct pt_scheduled_span){&p->master,sizeof(p->master)};
    p->spans[1]=(struct pt_scheduled_span){data,16};
    p->spans[2]=(struct pt_scheduled_span){p,sizeof(*p)};
    p->owner=(struct pt_scheduled_owner){p,token,current,retired,p->spans,3};
}
static void finish(struct prepared *p)
{if(p->held)retired(p,p->token);assert(pt_cache_clear(&p->cache));assert(p->memory.allocs==p->memory.frees);}
struct backend {
    uint64_t now;uint32_t frequency;int submit,poll,cancel,clock;
    unsigned submissions,polls,cancels,reads;const struct pt_scheduled_event *event[8];
    struct pt_scheduled_receipt receipt;struct pt_scheduled_output *reenter;int reentry;
};
static int read_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct backend *b=context;++b->reads;*ticks=b->now;*frequency=b->frequency;return b->clock;}
static int submit(void *context,const struct pt_scheduled_event *e)
{struct backend *b=context;assert(b->submissions<8);b->event[b->submissions++]=e;if(b->reenter)b->reentry=pt_scheduled_output_publish(b->reenter,e->ticket);return b->submit;}
static int poll(void *context,uint64_t ticket,struct pt_scheduled_receipt *r)
{struct backend *b=context;(void)ticket;++b->polls;*r=b->receipt;return b->poll;}
static int cancel(void *context,uint64_t ticket)
{struct backend *b=context;(void)ticket;++b->cancels;return b->cancel;}
static struct pt_scheduled_backend api(struct backend *b)
{return (struct pt_scheduled_backend){b,sizeof(*b),{PT_SCHEDULED_REQUIRED,8,64},read_clock,submit,poll,cancel};}
static struct pt_scheduled_batch batch(struct prepared *p,uint64_t frame)
{
    struct pt_scheduled_batch b;memset(&b,0,sizeof(b));b.generation=7;b.frame=frame;b.count=3;
    b.action[0]=(struct pt_scheduled_action){PT_SCHEDULED_TRIGGER,0,pt_cache_data(&p->cache,p->lease),8,124,64};
    b.action[1]=(struct pt_scheduled_action){PT_SCHEDULED_CONTROL,0,NULL,0,125,63};
    b.action[2]=(struct pt_scheduled_action){PT_SCHEDULED_STOP,0,NULL,0,0,0};return b;
}
static struct pt_scheduled_output *open_queue(struct pool *p,struct backend *b,unsigned n)
{
    struct pt_allocator a={p,allocate,queue_free};struct pt_scheduled_grid g={100,7,709379,48000};
    struct pt_scheduled_backend v=api(b);struct pt_scheduled_output *q=NULL;
    assert(pt_scheduled_output_open(&a,&g,&v,n,&q)==PT_SCHEDULED_OK);return q;
}
static void unsupported_and_open_failure(void)
{
    struct pool p={0};struct backend b={0};struct pt_scheduled_backend v=api(&b);
    struct pt_allocator a={&p,allocate,queue_free};struct pt_scheduled_grid g={100,7,709379,48000};
    struct pt_scheduled_output *q=(void *)(uintptr_t)1;unsigned flags;
    for(flags=0;flags<8;++flags)if(flags!=PT_SCHEDULED_REQUIRED) {
        v.caps.flags=flags;assert(pt_scheduled_output_open(&a,&g,&v,2,&q)==PT_SCHEDULED_UNSUPPORTED);
        assert(q==(void *)(uintptr_t)1 && !p.allocs && !b.submissions);
    }
    v.caps.flags=8;assert(pt_scheduled_output_open(&a,&g,&v,2,&q)==PT_SCHEDULED_UNSUPPORTED && !p.allocs);
    v=api(&b);v.submit=NULL;assert(pt_scheduled_output_open(&a,&g,&v,2,&q)==PT_SCHEDULED_UNSUPPORTED);
    v=api(&b);p.fail=1;assert(pt_scheduled_output_open(&a,&g,&v,2,&q)==PT_SCHEDULED_CAPACITY && q==(void *)(uintptr_t)1);
    p.fail=0;g.frequency=1;assert(pt_scheduled_output_open(&a,&g,&v,2,&q)==PT_SCHEDULED_INVALID);
    g=(struct pt_scheduled_grid){100,7,709379,48000};
    v.context=NULL;assert(pt_scheduled_output_open(&a,&g,&v,2,&q)==PT_SCHEDULED_INVALID);
    v=api(&b);v.context_bytes=0;assert(pt_scheduled_output_open(&a,&g,&v,2,&q)==PT_SCHEDULED_INVALID);
    v=api(&b);v.context=(void *)(UINTPTR_MAX-3);v.context_bytes=16;
    assert(pt_scheduled_output_open(&a,&g,&v,2,&q)==PT_SCHEDULED_INVALID);
    v=api(&b);assert(pt_scheduled_output_open(&a,&g,&v,2,(struct pt_scheduled_output **)(UINTPTR_MAX-3))==PT_SCHEDULED_INVALID);
    assert(pt_scheduled_output_open(&a,&g,&v,2,(struct pt_scheduled_output **)&g)==PT_SCHEDULED_INVALID);
    assert(pt_scheduled_output_open(&a,&g,&v,2,(struct pt_scheduled_output **)&b.now)==PT_SCHEDULED_INVALID && b.now==0);
    assert(!p.allocs);
}
struct arena {
    union {uint64_t alignment;struct pt_scheduled_output *handle;uint8_t bytes[65536];} storage;
    unsigned allocations,releases;
};
static void *arena_allocate(void *context,size_t n)
{struct arena *a=context;assert(n<=sizeof(a->storage));++a->allocations;return &a->storage;}
static void arena_release(void *context,void *p)
{struct arena *a=context;assert(p==&a->storage);++a->releases;}
static void allocated_output_alias(void)
{
    struct arena a;struct pt_allocator allocator={&a,arena_allocate,arena_release};
    struct backend b={0};struct pt_scheduled_backend v=api(&b);struct pt_scheduled_grid g={100,7,709379,48000};
    uint8_t image[sizeof(a.storage)];memset(&a,0,sizeof(a));a.storage.handle=(void *)(uintptr_t)99;
    memcpy(image,&a.storage,sizeof(image));
    assert(pt_scheduled_output_open(&allocator,&g,&v,2,&a.storage.handle)==PT_SCHEDULED_INVALID);
    assert(a.allocations==1 && a.releases==1 && !memcmp(image,&a.storage,sizeof(image)));
}
static void refusals(void)
{
    struct prepared p,other;struct pool memory={0};struct backend b={.now=100,.frequency=709379,.clock=1,.submit=1};
    struct pt_scheduled_output *q=open_queue(&memory,&b,1);struct pt_scheduled_batch event,before;
    uint64_t ticket=99;uint8_t *image=malloc(memory.bytes);unsigned checks;
    prepare(&p,1);prepare(&other,2);event=batch(&p,100);before=event;
    memcpy(image,memory.live,memory.bytes);
    assert(pt_scheduled_output_enqueue(q,&event,&p.owner,&p.master.scalar)==PT_SCHEDULED_INVALID);
    assert(pt_scheduled_output_enqueue(q,&event,&p.owner,(uint64_t *)((uint8_t *)&p.master+sizeof(p.master)-8))==PT_SCHEDULED_INVALID);
    assert(pt_scheduled_output_enqueue(q,&event,&p.owner,(uint64_t *)pt_cache_data(&p.cache,p.lease))==PT_SCHEDULED_INVALID);
    assert(pt_scheduled_output_enqueue(q,&event,&p.owner,(uint64_t *)memory.live)==PT_SCHEDULED_INVALID);
    assert(pt_scheduled_output_enqueue(q,&event,&p.owner,&b.now)==PT_SCHEDULED_INVALID && b.now==100);
    assert(pt_scheduled_output_enqueue(q,&event,&p.owner,(uint64_t *)(UINTPTR_MAX-3))==PT_SCHEDULED_INVALID);
    assert(!memcmp(image,memory.live,memory.bytes) && !memcmp(&event,&before,sizeof(event)) && !p.checks);
    event.frame=UINT64_MAX;assert(pt_scheduled_output_enqueue(q,&event,&p.owner,&ticket)==PT_SCHEDULED_CLOCK && ticket==99);
    event=before;event.action[0].data=(void *)(UINTPTR_MAX-1);
    assert(pt_scheduled_output_enqueue(q,&event,&p.owner,&ticket)==PT_SCHEDULED_INVALID);
    event=before;p.spans[0]=(struct pt_scheduled_span){(void *)(UINTPTR_MAX-3),16};
    assert(pt_scheduled_output_enqueue(q,&event,&p.owner,&ticket)==PT_SCHEDULED_INVALID);
    p.spans[0]=(struct pt_scheduled_span){&p.master,sizeof(p.master)};p.valid=0;
    assert(pt_scheduled_output_enqueue(q,&event,&p.owner,&ticket)==PT_SCHEDULED_STALE);
    p.valid=1;assert(!memcmp(image,memory.live,memory.bytes));
    assert(pt_scheduled_output_enqueue(q,&event,&p.owner,&ticket)==PT_SCHEDULED_OK && ticket==1);
    memcpy(image,memory.live,memory.bytes);checks=other.checks;event=batch(&other,200);
    assert(pt_scheduled_output_enqueue(q,&event,&other.owner,&p.master.scalar)==PT_SCHEDULED_INVALID); /* previous owner's capacity */
    assert(pt_scheduled_output_enqueue(q,&event,&other.owner,&ticket)==PT_SCHEDULED_CAPACITY);
    assert(!memcmp(image,memory.live,memory.bytes) && other.checks==checks && p.held && other.held);
    assert(!pt_scheduled_output_close(q));assert(pt_scheduled_output_stop(q)==PT_SCHEDULED_OK);
    assert(p.released==1 && !b.submissions && pt_scheduled_output_close(q));free(image);finish(&p);finish(&other);
}
static void future_publication_and_retirement(void)
{
    struct prepared p[3];struct pool memory={0};struct backend b={.now=100,.frequency=709379,.clock=1,.submit=1,.poll=0,.cancel=0};
    struct pt_scheduled_output *q=open_queue(&memory,&b,3);uint64_t tickets[3];unsigned i;struct pt_scheduled_event saved[3];
    for(i=0;i<3;++i){struct pt_scheduled_batch e;prepare(p+i,i+1);e=batch(p+i,100+i*100);
        assert(pt_scheduled_output_enqueue(q,&e,&p[i].owner,tickets+i)==PT_SCHEDULED_OK);}
    assert(pt_scheduled_output_publish(q,tickets[1])==PT_SCHEDULED_INVALID && !b.submissions);
    b.reenter=q;
    for(i=0;i<3;++i){assert(pt_scheduled_output_publish(q,tickets[i])==PT_SCHEDULED_OK);saved[i]=*b.event[i];}
    assert(b.reentry==PT_SCHEDULED_INVALID && memory.allocs==1 && memory.frees==0);
    assert(saved[0].first==100+1478 && saved[0].last==100+1493); /* ceil100*709379/48000 */
    for(i=0;i<3;++i){assert(p[i].memory.allocs==1 && p[i].memory.frees==0);assert(pt_cache_trim(&p[i].cache,16)==0);
        pt_cache_invalidate(&p[i].cache,p[i].token);assert(pt_cache_data(&p[i].cache,p[i].lease));}
    assert(pt_scheduled_output_poll(q,tickets[0])==PT_SCHEDULED_PENDING && p[0].held);
    b.poll=-1;assert(pt_scheduled_output_poll(q,tickets[0])==PT_SCHEDULED_BACKEND && p[0].held);
    assert(pt_scheduled_output_publish(q,tickets[0])==PT_SCHEDULED_INVALID); /* never retry */
    b.poll=1;b.receipt=(struct pt_scheduled_receipt){PT_SCHEDULED_EXECUTED,saved[2].first,saved[2].last-1};
    assert(pt_scheduled_output_poll(q,tickets[2])==PT_SCHEDULED_OK && p[2].released==1); /* out of order */
    assert(!memcmp(b.event[0],saved,sizeof(saved[0])) && !memcmp(b.event[1],saved+1,sizeof(saved[1])));
    assert(pt_scheduled_output_stop(q)==PT_SCHEDULED_PENDING && p[0].held && p[1].held && b.cancels==2);
    b.cancel=-1;assert(pt_scheduled_output_stop(q)==PT_SCHEDULED_BACKEND && p[0].held && p[1].held);
    b.cancel=2;assert(pt_scheduled_output_stop(q)==PT_SCHEDULED_BACKEND && p[0].held && p[1].held);
    b.cancel=1;assert(pt_scheduled_output_stop(q)==PT_SCHEDULED_OK && !pt_scheduled_output_held(q));
    assert(pt_scheduled_output_close(q) && memory.allocs==memory.frees);
    for(i=0;i<3;++i){assert(p[i].released==1);finish(p+i);}
}
static void timing_and_uncertain_submission(void)
{
    int mode;
    for(mode=0;mode<7;++mode) {
        struct prepared p;struct pool memory={0};struct backend b={.now=100,.frequency=709379,.clock=1,.submit=1,.poll=1,.cancel=1};
        struct pt_scheduled_output *q=open_queue(&memory,&b,1);struct pt_scheduled_batch e;uint64_t t;
        uint8_t master[sizeof(p.master)];prepare(&p,1);e=batch(&p,100);memcpy(master,&p.master,sizeof(master));
        assert(pt_scheduled_output_enqueue(q,&e,&p.owner,&t)==PT_SCHEDULED_OK);
        if(mode==0){p.valid=0;assert(pt_scheduled_output_publish(q,t)==PT_SCHEDULED_STALE && !b.submissions);}
        if(mode==1){b.frequency=715909;assert(pt_scheduled_output_publish(q,t)==PT_SCHEDULED_CLOCK && !b.submissions);}
        if(mode==2){b.now=1578;assert(pt_scheduled_output_publish(q,t)==PT_SCHEDULED_LATE && !b.submissions);}
        if(mode==3){b.submit=0;assert(pt_scheduled_output_publish(q,t)==PT_SCHEDULED_PENDING && p.held && b.submissions==1);
            b.submit=1;assert(pt_scheduled_output_publish(q,t)==PT_SCHEDULED_OK && b.submissions==2);}
        if(mode==4){b.submit=-1;assert(pt_scheduled_output_publish(q,t)==PT_SCHEDULED_BACKEND && p.held);
            assert(pt_scheduled_output_publish(q,t)==PT_SCHEDULED_INVALID && b.submissions==1);}
        if(mode==5 || mode==6){assert(pt_scheduled_output_publish(q,t)==PT_SCHEDULED_OK);
            b.receipt=(struct pt_scheduled_receipt){PT_SCHEDULED_EXECUTED,1578,mode==5?1592:1593};
            assert(pt_scheduled_output_poll(q,t)==(mode==5?PT_SCHEDULED_OK:PT_SCHEDULED_BACKEND));assert(p.released==1);}
        assert(!memcmp(master,&p.master,sizeof(master)) && memory.allocs==1);
        assert(pt_scheduled_output_stop(q)==PT_SCHEDULED_OK && p.released==1);assert(pt_scheduled_output_close(q));finish(&p);
    }
}
static void frame_windows_and_overflow(void)
{
    uint32_t frequencies[2]={709379,715909},rates[2]={44100,48000};unsigned f,r;
    for(f=0;f<2;++f)for(r=0;r<2;++r) {
        struct pool memory={0};struct prepared p;
        struct backend b={.now=100,.clock=1,.submit=1,.poll=1,.cancel=1};
        struct pt_allocator a={&memory,allocate,queue_free};struct pt_scheduled_backend v=api(&b);
        struct pt_scheduled_grid g={100,7,frequencies[f],rates[r]};struct pt_scheduled_output *q=NULL;
        struct pt_scheduled_batch e;uint64_t t,first,last,frame;
        b.frequency=g.frequency;assert(pt_scheduled_output_open(&a,&g,&v,1,&q)==PT_SCHEDULED_OK);
        for(frame=1;frame<=20;++frame) {
            prepare(&p,frame);e=batch(&p,frame);b.submissions=0;
            assert(pt_scheduled_output_enqueue(q,&e,&p.owner,&t)==PT_SCHEDULED_OK);
            assert(pt_scheduled_output_publish(q,t)==PT_SCHEDULED_OK);
            first=100+(frame*g.frequency+g.rate-1)/g.rate;last=100+((frame+1)*g.frequency+g.rate-1)/g.rate;
            assert(b.event[0]->first==first && b.event[0]->last==last && b.event[0]->batch.frame==frame);
            b.receipt=(struct pt_scheduled_receipt){PT_SCHEDULED_EXECUTED,first,last-1};
            assert(pt_scheduled_output_poll(q,t)==PT_SCHEDULED_OK && p.released==1);finish(&p);
        }
        assert(pt_scheduled_output_close(q) && memory.allocs==memory.frees);
        g.epoch=UINT64_MAX-100;assert(pt_scheduled_output_open(&a,&g,&v,1,&q)==PT_SCHEDULED_OK);
        prepare(&p,1);e=batch(&p,100);t=99;
        assert(pt_scheduled_output_enqueue(q,&e,&p.owner,&t)==PT_SCHEDULED_CLOCK && t==99 && p.held && !p.checks);
        assert(pt_scheduled_output_close(q));finish(&p);
    }
}
static void boundary_guards_and_partial_stop(void)
{
    struct pool memory={0};struct prepared p[3];struct backend b={.now=100,.frequency=709379,.clock=1,.submit=1,.poll=1,.cancel=0};
    struct pt_scheduled_output *q=open_queue(&memory,&b,3);struct pt_scheduled_batch e;
    uint64_t t[3];uint8_t *image;unsigned i;
    for(i=0;i<3;++i){prepare(p+i,i+1);e=batch(p+i,100+i*100);assert(pt_scheduled_output_enqueue(q,&e,&p[i].owner,t+i)==PT_SCHEDULED_OK);}
    assert(pt_scheduled_output_publish(q,t[0])==PT_SCHEDULED_OK);
    image=malloc(memory.bytes);assert(image);memcpy(image,memory.live,memory.bytes);
    b.now=99;assert(pt_scheduled_output_publish(q,t[1])==PT_SCHEDULED_CLOCK && b.submissions==1);
    assert(!memcmp(image,memory.live,memory.bytes));b.now=100;p[1].generation=8;
    assert(pt_scheduled_output_publish(q,t[1])==PT_SCHEDULED_STALE && b.submissions==1);
    assert(!memcmp(image,memory.live,memory.bytes));p[1].generation=7;
    pt_cache_invalidate(&p[1].cache,p[1].token);
    assert(pt_scheduled_output_publish(q,t[1])==PT_SCHEDULED_STALE && b.submissions==1);
    assert(!memcmp(image,memory.live,memory.bytes));
    assert(pt_scheduled_output_stop(q)==PT_SCHEDULED_PENDING);
    assert(p[0].held && p[1].released==1 && p[2].released==1 && b.cancels==1 && pt_scheduled_output_held(q)==1);
    assert(!pt_scheduled_output_close(q));b.poll=1;b.receipt=(struct pt_scheduled_receipt){PT_SCHEDULED_CANCELLED,0,0};
    assert(pt_scheduled_output_poll(q,t[0])==PT_SCHEDULED_OK && p[0].released==1);
    assert(pt_scheduled_output_close(q));free(image);for(i=0;i<3;++i)finish(p+i);
}
int main(void)
{
    unsupported_and_open_failure();allocated_output_alias();refusals();future_publication_and_retirement();timing_and_uncertain_submission();
    frame_windows_and_overflow();boundary_guards_and_partial_stop();
    puts("SCHEDULED OUTPUT PASS: explicit future capability, original frame windows, held master/cache capacities, bounded publication and confirmed retirement; no hardware timing proof");return 0;
}
