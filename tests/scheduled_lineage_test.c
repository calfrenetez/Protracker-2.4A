#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/core/scheduled_lineage.h"
#include "../src/core/document.h"
struct memory {void *p;size_t bytes;unsigned allocs,frees,fail;};
static void *allocate(void *v,size_t n)
{struct memory *m=v;if(m->fail)return NULL;m->p=malloc(n);assert(m->p);m->bytes=n;++m->allocs;return m->p;}
static void deallocate(void *v,void *p)
{struct memory *m=v;assert(p==m->p);++m->frees;free(p);m->p=NULL;}
struct holder {
    union {uint64_t scalar;int32_t precision[256];struct pt_lineage_receipt receipt;struct pt_lineage_key key;} master;
    union {uint64_t scalar;uint8_t bytes[64];} cache;
    unsigned held,released,notified,valid,terminal_good;
    struct pt_lineage_receipt terminal;
    struct pt_scheduled_span spans[3];struct pt_lineage_owner owner;
};
static int current(void *v,uint64_t token,uint64_t generation)
{struct holder *h=v;return h->held&&h->valid&&token==h->owner.held.token&&generation==7;}
static void released(void *v,uint64_t token)
{struct holder *h=v;assert(h->held&&h->owner.held.token==token);h->held=0;++h->released;}
static void notified(void *v,uint64_t token,const struct pt_lineage_receipt *r,int good)
{struct holder *h=v;assert(h->held&&h->owner.held.token==token);++h->notified;h->terminal=*r;h->terminal_good=(unsigned)good;}
static void hold(struct holder *h,uint64_t token)
{
    unsigned i;memset(h,0,sizeof(*h));for(i=0;i<256;++i)h->master.precision[i]=(int32_t)(0x123401U+i);
    for(i=0;i<64;++i)h->cache.bytes[i]=(uint8_t)(i+1);
    h->held=h->valid=1;h->spans[0]=(struct pt_scheduled_span){&h->master,sizeof(h->master)};
    h->spans[1]=(struct pt_scheduled_span){&h->cache,sizeof(h->cache)};h->spans[2]=(struct pt_scheduled_span){h,sizeof(*h)};
    h->owner=(struct pt_lineage_owner){{h,token,current,released,h->spans,3},notified};
}
struct record {
    const struct pt_lineage_event *borrowed;struct pt_lineage_event event;
    struct pt_lineage_receipt receipt;enum pt_lineage_reply reply;
    const uint8_t *data[4];unsigned fired;
};
struct backend {
    uint64_t now;uint32_t frequency;int clock,submit;
    unsigned reads,submissions,polls,cancels,effects,count,retirements;
    struct record record[32];struct pt_lineage_key reader[4];const uint8_t *data[4];
    enum pt_lineage_reader state[4];
    struct pt_lineage_output *reenter;unsigned reentry;enum pt_scheduled_result reentry_result;
};
static int equal(const struct pt_lineage_key *a,const struct pt_lineage_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->ticket==b->ticket&&a->owner==b->owner&&
 a->serial==b->serial&&a->action==b->action&&a->slot==b->slot;}
static struct record *record(struct backend *b,uint64_t t)
{unsigned i;for(i=0;i<b->count;++i)if(b->record[i].event.scheduled.ticket==t)return b->record+i;assert(0);return NULL;}
static int clock_read(void *v,uint64_t *ticks,uint32_t *frequency)
{struct backend *b=v;++b->reads;*ticks=b->now;*frequency=b->frequency;return b->clock;}
static void envelope(struct record *r)
{
    const struct pt_lineage_event *e=&r->event;
    memset(&r->receipt,0,sizeof(r->receipt));r->receipt.queue=e->queue;r->receipt.session=e->session;
    r->receipt.generation=e->scheduled.batch.generation;r->receipt.ticket=e->scheduled.ticket;r->receipt.owner=e->owner;
    r->receipt.count=e->scheduled.batch.count;
}
static int condition(struct backend *b,const struct pt_lineage_event *e)
{
    unsigned i;for(i=0;i<e->scheduled.batch.count;++i)if(e->scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER){
        unsigned slot=e->scheduled.batch.action[i].slot;
        if(b->state[slot]!=PT_LINEAGE_ACTIVE||!equal(b->reader+slot,e->key+i))return 0;
    }
    for(i=0;i<e->scheduled.batch.count;++i)if(e->scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER){
        unsigned j,k;for(j=0;j<b->count;++j)if(b->record[j].borrowed&&!b->record[j].fired&&
           b->record[j].event.scheduled.batch.frame<e->scheduled.batch.frame)
            for(k=0;k<b->record[j].event.scheduled.batch.count;++k){
                const struct pt_scheduled_action *a=b->record[j].event.scheduled.batch.action+k;
                if(a->slot==e->key[i].slot&&(a->kind==PT_SCHEDULED_TRIGGER||a->kind==PT_SCHEDULED_STOP))return 0;
            }
    }
    return 1;
}
static int submit(void *v,const struct pt_lineage_event *e)
{
    struct backend *b=v;struct record *r;unsigned i;++b->submissions;
    assert(b->now<e->scheduled.first);
    if(b->reentry){b->reentry_result=pt_lineage_publish(b->reenter,e->scheduled.ticket);b->reentry=0;}
    if(!b->submit||!condition(b,e))return 0;
    assert(b->count<32);r=b->record+b->count++;memset(r,0,sizeof(*r));r->borrowed=e;r->event=*e;envelope(r);
    for(i=0;i<e->scheduled.batch.count;++i)if(e->scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER){
        r->receipt.action[i].key=e->key[i];r->receipt.action[i].reader=PT_LINEAGE_ACTIVE;r->data[i]=b->data[e->key[i].slot];
    }else r->data[i]=e->scheduled.batch.action[i].data;
    return b->submit;
}
static enum pt_lineage_reply poll(void *v,uint64_t t,struct pt_lineage_receipt *out)
{
    struct backend *b=v;struct record *r=record(b,t);++b->polls;*out=r->receipt;
    if(r->reply==PT_LINEAGE_ALL_RETIRED){assert(r->borrowed);r->borrowed=NULL;++b->retirements;}
    return r->reply;
}
static enum pt_lineage_reply cancel(void *v,uint64_t t,struct pt_lineage_receipt *out)
{struct backend *b=v;++b->cancels;return poll(v,t,out);}
static struct pt_lineage_backend api(struct backend *b)
{return (struct pt_lineage_backend){b,sizeof(*b),{7,8,4},1,1,clock_read,submit,poll,cancel};}
static struct pt_lineage_output *open(struct memory *m,struct backend *b,unsigned n,uint64_t session)
{
    struct pt_allocator a={m,allocate,deallocate};struct pt_scheduled_grid grid={100,7,709379,48000};
    struct pt_lineage_backend v=api(b);struct pt_lineage_output *q=NULL;
    b->now=100;b->frequency=grid.frequency;b->clock=b->submit=1;
    assert(pt_lineage_open(&a,&grid,session,&v,n,&q)==PT_SCHEDULED_OK);return q;
}
static struct pt_scheduled_batch batch(struct holder *h,uint64_t frame,unsigned slot,enum pt_scheduled_kind kind)
{
    struct pt_scheduled_batch b;memset(&b,0,sizeof(b));b.generation=7;b.frame=frame;b.count=1;
    b.action[0]=(struct pt_scheduled_action){kind,slot,kind==PT_SCHEDULED_TRIGGER?h->cache.bytes:NULL,
        kind==PT_SCHEDULED_TRIGGER?16:0,kind==PT_SCHEDULED_STOP?0:124,kind==PT_SCHEDULED_STOP?0:64};return b;
}
/* Injected backend repeats the COMPLETE conditional comparison at its actual
 * activation boundary. No effects occur unless every target and the clock pass. */
static void fire(struct backend *b,uint64_t ticket)
{
    struct record *r=record(b,ticket);const struct pt_lineage_event *e=&r->event;unsigned i;
    assert(!r->fired);r->fired=1;r->reply=PT_LINEAGE_OBSERVATION;
    if(b->now<e->scheduled.first||b->now>=e->scheduled.last||!condition(b,e)){
        for(i=0;i<e->scheduled.batch.count;++i){r->receipt.action[i].command=PT_LINEAGE_FAILED;
            if(e->scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER){r->receipt.action[i].key=e->key[i];r->receipt.action[i].reader=PT_LINEAGE_RETIRED;}}
        return;
    }
    for(i=0;i<e->scheduled.batch.count;++i){const struct pt_scheduled_action *a=e->scheduled.batch.action+i;
        struct pt_lineage_action_receipt *ract=r->receipt.action+i;
        ++b->effects;ract->command=PT_LINEAGE_ISSUED;ract->key=e->key[i];ract->observed=ract->issued=b->now;
        if(a->kind==PT_SCHEDULED_TRIGGER){b->reader[a->slot]=e->key[i];b->data[a->slot]=a->data;b->state[a->slot]=PT_LINEAGE_ACTIVE;}
        if(a->kind==PT_SCHEDULED_STOP)b->state[a->slot]=PT_LINEAGE_DRAINING;
        ract->reader=b->state[a->slot];
    }
}
static void drain(struct backend *b)
{unsigned i;for(i=0;i<4;++i){b->state[i]=PT_LINEAGE_RETIRED;b->data[i]=NULL;}}
static void retire(struct backend *b,uint64_t ticket,int cancelled)
{
    struct record *r=record(b,ticket);unsigned i,j;
    for(i=0;i<r->event.scheduled.batch.count;++i){struct pt_lineage_action_receipt *a=r->receipt.action+i;
        for(j=0;j<4;++j)assert(!b->data[j]||b->data[j]!=r->data[i]); /* shared-cache no-reader proof */
        if(!r->fired){a->command=PT_LINEAGE_CANCELLED_BEFORE;a->observed=a->issued=0;
            if(r->event.scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER){a->key=r->event.key[i];a->reader=PT_LINEAGE_RETIRED;}}
        else {if(cancelled)a->command=PT_LINEAGE_CANCELLED_AFTER;a->reader=PT_LINEAGE_RETIRED;}
    }
    r->reply=PT_LINEAGE_ALL_RETIRED;
}
static uint64_t trigger(struct pt_lineage_output *q,struct backend *b,struct holder *h,uint64_t frame,unsigned slot)
{
    struct pt_scheduled_batch e=batch(h,frame,slot,PT_SCHEDULED_TRIGGER);uint64_t t=0;struct pt_lineage_receipt receipt;
    assert(pt_lineage_enqueue(q,&e,NULL,&h->owner,&t)==PT_SCHEDULED_OK);
    assert(pt_lineage_publish(q,t)==PT_SCHEDULED_OK);b->now=record(b,t)->event.scheduled.first;fire(b,t);
    assert(pt_lineage_poll(q,t,&receipt)==PT_SCHEDULED_PENDING&&receipt.action[0].reader==PT_LINEAGE_ACTIVE&&h->held);
    return t;
}
static void close_all(struct pt_lineage_output *q,struct backend *b,struct holder *h,unsigned n)
{
    unsigned i;struct pt_lineage_receipt receipt;drain(b);
    for(i=0;i<b->count;++i)if(b->record[i].borrowed){retire(b,b->record[i].event.scheduled.ticket,0);
        (void)pt_lineage_poll(q,b->record[i].event.scheduled.ticket,&receipt);}
    (void)pt_lineage_stop(q);assert(!pt_lineage_held(q)&&pt_lineage_close(q));
    for(i=0;i<n;++i)assert(h[i].released==1&&h[i].notified==1&&!h[i].held);
}
static void observed_lifecycle(void)
{
    struct memory m={0};struct backend b={0};struct holder h[4];struct pt_lineage_output *q=open(&m,&b,8,23);
    struct pt_lineage_key key,sentinel;struct pt_lineage_receipt out,again;struct pt_scheduled_batch e;uint64_t t,s,c;
    unsigned i;for(i=0;i<4;++i)hold(h+i,i+1);memset(&sentinel,0x55,sizeof(sentinel));key=sentinel;
    e=batch(h,100,0,PT_SCHEDULED_TRIGGER);assert(pt_lineage_enqueue(q,&e,NULL,&h[0].owner,&t)==PT_SCHEDULED_OK);
    assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_STALE&&!memcmp(&key,&sentinel,sizeof(key)));
    assert(pt_lineage_publish(q,t)==PT_SCHEDULED_OK);assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_STALE);
    memset(&out,0x33,sizeof(out));again=out;assert(pt_lineage_poll(q,t,&out)==PT_SCHEDULED_PENDING&&!memcmp(&out,&again,sizeof(out)));
    b.now=record(&b,t)->event.scheduled.first;fire(&b,t);assert(pt_lineage_poll(q,t,&out)==PT_SCHEDULED_PENDING);
    assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_OK&&key.ticket==t&&key.serial);
    again=out;assert(pt_lineage_poll(q,t,&out)==PT_SCHEDULED_PENDING&&!memcmp(&out,&again,sizeof(out))); /* idempotent */
    e=batch(h+1,200,0,PT_SCHEDULED_CONTROL);assert(pt_lineage_enqueue(q,&e,&key,&h[1].owner,&c)==PT_SCHEDULED_OK);
    assert(pt_lineage_publish(q,c)==PT_SCHEDULED_OK);record(&b,c)->receipt.action[0].command=PT_LINEAGE_CANCELLED_BEFORE;
    record(&b,c)->reply=PT_LINEAGE_OBSERVATION;assert(pt_lineage_cancel(q,c,&out)==PT_SCHEDULED_PENDING&&h[1].held);
    assert(b.state[0]==PT_LINEAGE_ACTIVE&&pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_OK);
    sentinel=key;h[0].valid=0;
    assert(pt_lineage_reader_key(q,c,0,&sentinel)==PT_SCHEDULED_INVALID&&!memcmp(&sentinel,&key,sizeof(key))&&h[1].valid);
    h[0].valid=1;
    e=batch(h+2,300,0,PT_SCHEDULED_STOP);assert(pt_lineage_enqueue(q,&e,&key,&h[2].owner,&s)==PT_SCHEDULED_OK);
    e=batch(h+3,400,0,PT_SCHEDULED_CONTROL);i=99;{uint64_t unchanged=i;assert(pt_lineage_enqueue(q,&e,&key,&h[3].owner,&unchanged)==PT_SCHEDULED_INVALID&&unchanged==99);}
    assert(pt_lineage_reader_key(q,t,0,&sentinel)==PT_SCHEDULED_STALE);
    sentinel=key;assert(pt_lineage_reader_key(q,s,0,&sentinel)==PT_SCHEDULED_INVALID&&!memcmp(&sentinel,&key,sizeof(key)));
    assert(pt_lineage_publish(q,s)==PT_SCHEDULED_OK);record(&b,s)->receipt.action[0].reader=PT_LINEAGE_STOP_PENDING;
    record(&b,s)->reply=PT_LINEAGE_OBSERVATION;assert(pt_lineage_poll(q,s,&out)==PT_SCHEDULED_PENDING&&h[2].held);
    b.now=record(&b,s)->event.scheduled.first;fire(&b,s);assert(pt_lineage_poll(q,s,&out)==PT_SCHEDULED_PENDING&&out.action[0].reader==PT_LINEAGE_DRAINING);
    assert(h[0].held&&h[1].held&&h[2].held);drain(&b);retire(&b,s,0);
    assert(pt_lineage_poll(q,s,&out)==PT_SCHEDULED_OK&&h[2].released==1&&h[0].held&&h[1].held);
    retire(&b,t,0);assert(pt_lineage_poll(q,t,&out)==PT_SCHEDULED_OK&&h[0].released==1&&h[1].held);
    retire(&b,c,0);assert(pt_lineage_poll(q,c,&out)==PT_SCHEDULED_OK&&h[1].released==1);
    released(h+3,4);assert(pt_lineage_close(q)&&m.allocs==m.frees);
}
static void cancellation_history(void)
{
    unsigned mode;for(mode=0;mode<2;++mode){struct memory m={0};struct backend b={0};struct holder h;
        struct pt_lineage_output *q=open(&m,&b,1,30+mode);struct pt_scheduled_batch e;struct pt_lineage_receipt r;
        struct pt_lineage_key key;uint64_t t;hold(&h,1);e=batch(&h,100,0,PT_SCHEDULED_TRIGGER);
        assert(pt_lineage_enqueue(q,&e,NULL,&h.owner,&t)==PT_SCHEDULED_OK&&pt_lineage_publish(q,t)==PT_SCHEDULED_OK);
        if(mode){b.now=record(&b,t)->event.scheduled.first;fire(&b,t);}
        drain(&b);retire(&b,t,(int)mode);assert(pt_lineage_cancel(q,t,&r)==PT_SCHEDULED_OK);
        assert(r.action[0].command==(mode?PT_LINEAGE_CANCELLED_AFTER:PT_LINEAGE_CANCELLED_BEFORE));
        assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_INVALID&&h.released==1&&h.notified==1&&pt_lineage_close(q));}
}
static void exact_target_and_atomic_effects(void)
{
    struct memory m={0};struct backend b={0};struct holder h[4];struct pt_lineage_output *q=open(&m,&b,8,41);
    struct pt_lineage_key keys[2],bad;struct pt_scheduled_batch e;struct pt_lineage_receipt r;uint64_t t0,t1,c,replace;
    unsigned i,effects;for(i=0;i<4;++i)hold(h+i,i+1);
    t0=trigger(q,&b,h,100,0);t1=trigger(q,&b,h+1,200,1);
    assert(pt_lineage_reader_key(q,t0,0,keys)==PT_SCHEDULED_OK&&pt_lineage_reader_key(q,t1,0,keys+1)==PT_SCHEDULED_OK);
    e=batch(h+2,300,0,PT_SCHEDULED_CONTROL);bad=keys[0];bad.serial++;
    assert(pt_lineage_enqueue(q,&e,&bad,&h[2].owner,&c)==PT_SCHEDULED_INVALID);
    e.count=2;e.action[1]=e.action[0];e.action[1].slot=1;
    assert(pt_lineage_enqueue(q,&e,keys,&h[2].owner,&c)==PT_SCHEDULED_OK&&pt_lineage_publish(q,c)==PT_SCHEDULED_OK);
    /* Same address held by another trigger NEVER proves the original key. */
    e=batch(h+3,400,1,PT_SCHEDULED_TRIGGER);e.action[0].data=h[1].cache.bytes;
    h[3].spans[1]=(struct pt_scheduled_span){h[1].cache.bytes,sizeof(h[1].cache)};
    assert(pt_lineage_enqueue(q,&e,NULL,&h[3].owner,&replace)==PT_SCHEDULED_OK&&pt_lineage_publish(q,replace)==PT_SCHEDULED_OK);
    b.reader[1]=record(&b,replace)->event.key[0]; /* adversarial replacement between admission and activation */
    effects=b.effects;b.now=record(&b,c)->event.scheduled.first;fire(&b,c);
    assert(b.effects==effects&&pt_lineage_poll(q,c,&r)==PT_SCHEDULED_BACKEND&&h[2].held); /* complete zero effects */
    close_all(q,&b,h,4);assert(m.allocs==m.frees);
}
static void stale_before_publish(void)
{
    struct memory m={0};struct backend b={0};struct holder h[2];struct pt_lineage_output *q=open(&m,&b,2,51);
    struct pt_lineage_key key;struct pt_lineage_receipt r;struct pt_scheduled_batch e;uint64_t t,c;
    unsigned calls;hold(h,1);hold(h+1,2);t=trigger(q,&b,h,100,0);
    assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_OK);e=batch(h+1,200,0,PT_SCHEDULED_CONTROL);
    assert(pt_lineage_enqueue(q,&e,&key,&h[1].owner,&c)==PT_SCHEDULED_OK);calls=b.submissions;
    drain(&b);retire(&b,t,1);assert(pt_lineage_poll(q,t,&r)==PT_SCHEDULED_OK);
    assert(pt_lineage_publish(q,c)==PT_SCHEDULED_STALE&&b.submissions==calls&&h[1].held);
    assert(pt_lineage_cancel(q,c,&r)==PT_SCHEDULED_OK&&h[1].held==0);
    assert(r.provenance==PT_LINEAGE_LOCAL_UNSUBMITTED&&r.action[0].reader==PT_LINEAGE_READER_UNKNOWN);
    assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_INVALID&&pt_lineage_close(q));
}
static void receipt_failures(void)
{
    unsigned mode;for(mode=0;mode<15;++mode){struct memory m={0};struct backend b={0};struct holder h;
        struct pt_lineage_output *q=open(&m,&b,1,60+mode);struct pt_lineage_receipt r,sentinel,original;
        struct pt_scheduled_batch e;struct record *rec;uint64_t t;hold(&h,1);e=batch(&h,100,0,PT_SCHEDULED_TRIGGER);
        assert(pt_lineage_enqueue(q,&e,NULL,&h.owner,&t)==PT_SCHEDULED_OK&&pt_lineage_publish(q,t)==PT_SCHEDULED_OK);
        b.now=record(&b,t)->event.scheduled.first;fire(&b,t);rec=record(&b,t);original=rec->receipt;
        memset(&r,0x77,sizeof(r));sentinel=r;
        switch(mode){case 0:rec->receipt.queue=NULL;break;case 1:++rec->receipt.session;break;case 2:++rec->receipt.generation;break;
        case 3:++rec->receipt.ticket;break;case 4:++rec->receipt.owner;break;case 5:++rec->receipt.count;break;
        case 6:++rec->receipt.action[0].key.serial;break;case 7:++rec->receipt.action[0].key.action;break;
        case 8:++rec->receipt.action[0].key.slot;break;case 9:rec->receipt.action[0].issued=rec->event.scheduled.last;break;
        case 10:rec->receipt.action[0].observed=rec->receipt.action[0].issued+1;break;
        case 11:rec->receipt.action[0].command=PT_LINEAGE_UNKNOWN;break;
        case 12:rec->receipt.action[0].reader=PT_LINEAGE_READER_UNKNOWN;break;
        case 13:rec->reply=PT_LINEAGE_UNCERTAIN;break;case 14:rec->reply=(enum pt_lineage_reply)3;break;}
        assert(pt_lineage_poll(q,t,&r)==PT_SCHEDULED_BACKEND&&h.held&&!h.released&&!memcmp(&r,&sentinel,sizeof(r)));
        assert(!pt_lineage_close(q));rec->receipt=original;drain(&b);retire(&b,t,0);
        assert(pt_lineage_poll(q,t,&r)==PT_SCHEDULED_BACKEND&&!h.held&&h.released==1&&h.terminal_good&&pt_lineage_close(q));}
    /* Late/invalid action timing is separate from a correctly bound explicit
     * retirement acknowledgement; mismatch in retirement identity is NOT. */
    {struct memory m={0};struct backend b={0};struct holder h;struct pt_lineage_output *q=open(&m,&b,1,90);
        struct pt_lineage_receipt r;uint64_t t;struct record *rec;hold(&h,1);t=trigger(q,&b,&h,100,0);drain(&b);retire(&b,t,0);
        rec=record(&b,t);rec->receipt.ticket++;assert(pt_lineage_poll(q,t,&r)==PT_SCHEDULED_BACKEND&&h.held);
        /* Bad backend must still retain a reference until the correct receipt. */
        rec->borrowed=&rec->event;rec->receipt.ticket=t;rec->receipt.action[0].issued=rec->event.scheduled.last;
        assert(pt_lineage_poll(q,t,&r)==PT_SCHEDULED_BACKEND&&h.released==1&&h.notified==1&&pt_lineage_close(q));}
}
static void aliases_and_keys(void)
{
    struct memory m={0};struct backend b={0};struct holder h[2],before[2];struct pt_lineage_output *q=open(&m,&b,2,101);
    struct pt_lineage_key key,wrong,sentinel;struct pt_lineage_receipt out;struct pt_scheduled_batch e,saved;
    uint64_t t,c=99;void *image;unsigned i;hold(h,1);hold(h+1,2);t=trigger(q,&b,h,100,0);
    assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_OK);e=batch(h+1,200,0,PT_SCHEDULED_CONTROL);saved=e;
    image=malloc(m.bytes);assert(image);memcpy(image,m.p,m.bytes);memcpy(before,h,sizeof(h));
    assert(pt_lineage_enqueue(q,&e,&key,&h[1].owner,&h[0].master.scalar)==PT_SCHEDULED_INVALID);
    assert(pt_lineage_enqueue(q,&e,&key,&h[1].owner,(uint64_t *)((uint8_t *)&h[0].master+sizeof(h[0].master)-8))==PT_SCHEDULED_INVALID);
    assert(pt_lineage_enqueue(q,&e,&key,&h[1].owner,&h[1].cache.scalar)==PT_SCHEDULED_INVALID);
    assert(pt_lineage_enqueue(q,&e,&key,&h[1].owner,&b.now)==PT_SCHEDULED_INVALID);
    assert(pt_lineage_enqueue(q,&e,&key,&h[1].owner,(uint64_t *)(UINTPTR_MAX-3))==PT_SCHEDULED_INVALID);
    assert(pt_lineage_poll(q,t,&h[0].master.receipt)==PT_SCHEDULED_INVALID);
    assert(pt_lineage_reader_key(q,t,0,&h[0].master.key)==PT_SCHEDULED_INVALID);
    assert(pt_lineage_enqueue(q,(struct pt_scheduled_batch *)(UINTPTR_MAX-3),&key,&h[1].owner,&c)==PT_SCHEDULED_INVALID);
    assert(pt_lineage_enqueue(q,&e,(struct pt_lineage_key *)(UINTPTR_MAX-3),&h[1].owner,&c)==PT_SCHEDULED_INVALID);
    assert(pt_lineage_enqueue(q,&e,&key,(struct pt_lineage_owner *)(UINTPTR_MAX-3),&c)==PT_SCHEDULED_INVALID);
    assert(!memcmp(image,m.p,m.bytes)&&!memcmp(before,h,sizeof(h))&&!memcmp(&saved,&e,sizeof(e))&&c==99);
    for(i=0;i<8;++i){wrong=key;switch(i){case 0:wrong.queue=NULL;break;case 1:wrong.session++;break;case 2:wrong.generation++;break;
        case 3:wrong.ticket++;break;case 4:wrong.owner++;break;case 5:wrong.serial++;break;case 6:wrong.action++;break;case 7:wrong.slot++;break;}
        assert(pt_lineage_enqueue(q,&e,&wrong,&h[1].owner,&c)==PT_SCHEDULED_INVALID&&c==99);}
    sentinel=key;h[0].valid=0;assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_STALE&&!memcmp(&key,&sentinel,sizeof(key)));
    assert(pt_lineage_enqueue(q,&e,&key,&h[1].owner,&c)==PT_SCHEDULED_STALE&&c==99);h[0].valid=1;
    e.count=2;e.action[1]=e.action[0];assert(pt_lineage_enqueue(q,&e,&key,&h[1].owner,&c)==PT_SCHEDULED_INVALID);e=saved;
    assert(pt_lineage_enqueue(q,&e,&key,&h[1].owner,&c)==PT_SCHEDULED_OK);assert(pt_lineage_cancel(q,c,&out)==PT_SCHEDULED_OK);
    free(image);drain(&b);retire(&b,t,0);assert(pt_lineage_poll(q,t,&out)==PT_SCHEDULED_OK&&pt_lineage_close(q));
}
static void pressure_and_uncertainty(void)
{
    struct memory m={0};struct backend b={0};struct holder h[9];struct pt_lineage_output *q=open(&m,&b,8,120);
    struct pt_lineage_key key;struct pt_scheduled_batch e;struct pt_lineage_receipt r;uint64_t tickets[8],t=99;unsigned i,allocations;
    void *image;for(i=0;i<9;++i)hold(h+i,i+1);tickets[0]=trigger(q,&b,h,100,0);
    assert(pt_lineage_reader_key(q,tickets[0],0,&key)==PT_SCHEDULED_OK);allocations=m.allocs;
    for(i=1;i<8;++i){e=batch(h+i,100+i*100,0,PT_SCHEDULED_CONTROL);
        assert(pt_lineage_enqueue(q,&e,&key,&h[i].owner,tickets+i)==PT_SCHEDULED_OK&&pt_lineage_publish(q,tickets[i])==PT_SCHEDULED_OK);
        record(&b,tickets[i])->receipt.action[0].command=PT_LINEAGE_CANCELLED_BEFORE;record(&b,tickets[i])->reply=PT_LINEAGE_OBSERVATION;
        assert(pt_lineage_cancel(q,tickets[i],&r)==PT_SCHEDULED_PENDING&&h[i].held&&b.state[0]==PT_LINEAGE_ACTIVE);}
    e=batch(h+8,1000,0,PT_SCHEDULED_CONTROL);image=malloc(m.bytes);assert(image);memcpy(image,m.p,m.bytes);
    assert(pt_lineage_enqueue(q,&e,&key,&h[8].owner,&t)==PT_SCHEDULED_CAPACITY&&t==99&&!memcmp(image,m.p,m.bytes));
    assert(m.allocs==allocations&&pt_lineage_held(q)==8);free(image);released(h+8,9);
    record(&b,tickets[0])->reply=PT_LINEAGE_UNCERTAIN;assert(pt_lineage_stop(q)==PT_SCHEDULED_BACKEND&&pt_lineage_held(q)==8);
    assert(!pt_lineage_close(q));close_all(q,&b,h,8);
}
static void open_and_clock(void)
{
    struct memory m={0};struct backend b={0};struct pt_lineage_backend v=api(&b);struct pt_allocator a={&m,allocate,deallocate};
    struct pt_scheduled_grid g={100,7,709379,48000};struct pt_lineage_output *q=(void *)(uintptr_t)9;unsigned i;
    for(i=0;i<9;++i){v=api(&b);switch(i){case 0:v.caps.flags=15;break;case 1:v.version=2;break;case 2:v.lineage_flags=3;break;
        case 3:v.caps.maximum_actions=5;break;case 4:v.poll=NULL;break;case 5:v.cancel=NULL;break;
        case 6:v.submit=NULL;break;case 7:v.read_clock=NULL;break;case 8:v.caps.maximum_batches=9;break;}
        assert(pt_lineage_open(&a,&g,1,&v,1,&q)==PT_SCHEDULED_UNSUPPORTED&&q==(void *)(uintptr_t)9&&!m.allocs);}
    v=api(&b);assert(pt_lineage_open(&a,&g,0,&v,1,&q)==PT_SCHEDULED_INVALID);
    v.context_bytes=0;assert(pt_lineage_open(&a,&g,1,&v,1,&q)==PT_SCHEDULED_INVALID);
    v=api(&b);v.context=(void *)(UINTPTR_MAX-3);assert(pt_lineage_open(&a,&g,1,&v,1,&q)==PT_SCHEDULED_INVALID);
    v=api(&b);assert(pt_lineage_open(&a,&g,1,&v,1,(struct pt_lineage_output **)&b.now)==PT_SCHEDULED_INVALID);
    assert(pt_lineage_open(&a,(struct pt_scheduled_grid *)(UINTPTR_MAX-3),1,&v,1,&q)==PT_SCHEDULED_INVALID);
    assert(pt_lineage_open(&a,&g,1,(struct pt_lineage_backend *)(UINTPTR_MAX-3),1,&q)==PT_SCHEDULED_UNSUPPORTED);
    m.fail=1;assert(pt_lineage_open(&a,&g,1,&v,1,&q)==PT_SCHEDULED_CAPACITY);assert(!m.allocs);
    for(i=0;i<5;++i){struct holder h;struct pt_scheduled_batch e;struct pt_lineage_receipt r;uint64_t t;
        m.fail=0;memset(&b,0,sizeof(b));q=open(&m,&b,1,140+i);hold(&h,1);e=batch(&h,100,0,PT_SCHEDULED_TRIGGER);
        assert(pt_lineage_enqueue(q,&e,NULL,&h.owner,&t)==PT_SCHEDULED_OK);
        if(i==0){b.now=1578;assert(pt_lineage_publish(q,t)==PT_SCHEDULED_LATE);b.now=100;assert(pt_lineage_publish(q,t)==PT_SCHEDULED_INVALID&&!b.submissions);}
        if(i==1){b.frequency=1;assert(pt_lineage_publish(q,t)==PT_SCHEDULED_CLOCK);b.frequency=709379;assert(pt_lineage_publish(q,t)==PT_SCHEDULED_INVALID&&!b.submissions);}
        if(i==2){b.submit=0;assert(pt_lineage_publish(q,t)==PT_SCHEDULED_PENDING&&h.held);b.submit=1;assert(pt_lineage_publish(q,t)==PT_SCHEDULED_OK);}
        if(i==3){b.submit=-1;assert(pt_lineage_publish(q,t)==PT_SCHEDULED_BACKEND&&h.held);assert(pt_lineage_publish(q,t)==PT_SCHEDULED_INVALID);}
        if(i==4){b.reenter=q;b.reentry=1;assert(pt_lineage_publish(q,t)==PT_SCHEDULED_BACKEND&&b.reentry_result==PT_SCHEDULED_BACKEND&&h.held);}
        if(b.count){drain(&b);retire(&b,t,0);(void)pt_lineage_poll(q,t,&r);}else assert(pt_lineage_cancel(q,t,&r)==PT_SCHEDULED_OK);
        assert(h.released==1&&pt_lineage_close(q));}
}
struct reusable {
    union {uint64_t align;struct pt_lineage_output *handle;uint8_t bytes[131072];} storage;
    unsigned allocs,frees;
};
static void *reuse_allocate(void *v,size_t n)
{struct reusable *a=v;assert(n<=sizeof(a->storage));++a->allocs;return &a->storage;}
static void reuse_free(void *v,void *p)
{struct reusable *a=v;assert(p==&a->storage);++a->frees;}
static void session_and_allocated_alias(void)
{
    struct reusable arena;struct backend b={0};struct pt_lineage_backend v=api(&b);
    struct pt_allocator a={&arena,reuse_allocate,reuse_free};struct pt_scheduled_grid g={100,7,709379,48000};
    struct pt_lineage_output *q,*previous;struct holder h[2];struct pt_lineage_key key,old;
    struct pt_lineage_receipt out;struct pt_scheduled_batch e;uint64_t t,c=99;uint8_t *image=malloc(sizeof(arena.storage));
    assert(image);memset(&arena,0,sizeof(arena));arena.storage.handle=(void *)(uintptr_t)99;
    memcpy(image,&arena.storage,sizeof(arena.storage));
    assert(pt_lineage_open(&a,&g,200,&v,2,&arena.storage.handle)==PT_SCHEDULED_INVALID&&arena.allocs==1&&arena.frees==1);
    assert(!memcmp(image,&arena.storage,sizeof(arena.storage)));free(image);
    b.now=100;b.frequency=709379;b.clock=b.submit=1;hold(h,1);hold(h+1,2);
    assert(pt_lineage_open(&a,&g,201,&v,2,&q)==PT_SCHEDULED_OK);previous=q;
    t=trigger(q,&b,h,100,0);assert(pt_lineage_reader_key(q,t,0,&old)==PT_SCHEDULED_OK);
    drain(&b);retire(&b,t,0);assert(pt_lineage_poll(q,t,&out)==PT_SCHEDULED_OK&&pt_lineage_close(q));
    memset(&b,0,sizeof(b));b.now=100;b.frequency=709379;b.clock=b.submit=1;v=api(&b);hold(h,3);
    assert(pt_lineage_open(&a,&g,202,&v,2,&q)==PT_SCHEDULED_OK&&q==previous);
    t=trigger(q,&b,h,100,0);assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_OK&&key.serial==old.serial&&key.session!=old.session);
    e=batch(h+1,200,0,PT_SCHEDULED_CONTROL);assert(pt_lineage_enqueue(q,&e,&old,&h[1].owner,&c)==PT_SCHEDULED_INVALID&&c==99);
    close_all(q,&b,h,1);released(h+1,2);assert(arena.allocs==arena.frees);
}
static void contradictions_and_mutable_observation(void)
{
    unsigned mode;for(mode=0;mode<6;++mode){struct memory m={0};struct backend b={0};struct holder h;
        struct pt_lineage_output *q=open(&m,&b,1,220+mode);struct pt_lineage_receipt out;struct record *r;uint64_t t;
        hold(&h,1);t=trigger(q,&b,&h,100,0);r=record(&b,t);
        if(mode==0)r->receipt.action[0].command=PT_LINEAGE_CANCELLED_BEFORE; /* never-issued + ACTIVE contradiction */
        if(mode==1)++r->receipt.action[0].issued; /* still inside window, but duplicate immutable timing changed */
        if(mode==2)++r->receipt.action[0].key.owner;
        if(mode==3)r->receipt.action[0].command=PT_LINEAGE_WAITING;
        if(mode==4)r->receipt.action[0].reader=PT_LINEAGE_NONE;
        if(mode==5){r->receipt.action[0].reader=PT_LINEAGE_DRAINING;assert(pt_lineage_poll(q,t,&out)==PT_SCHEDULED_PENDING);r->receipt.action[0].reader=PT_LINEAGE_ACTIVE;}
        assert(pt_lineage_poll(q,t,&out)==PT_SCHEDULED_BACKEND&&h.held);close_all(q,&b,&h,1);}
    {struct memory m={0};struct backend b={0};struct holder h;struct pt_lineage_output *q=open(&m,&b,1,240);
        struct pt_scheduled_batch e;struct pt_lineage_receipt out;struct record *r;uint64_t t;
        hold(&h,1);e=batch(&h,100,0,PT_SCHEDULED_TRIGGER);e.count=2;e.action[1]=e.action[0];e.action[1].slot=1;
        assert(pt_lineage_enqueue(q,&e,NULL,&h.owner,&t)==PT_SCHEDULED_OK&&pt_lineage_publish(q,t)==PT_SCHEDULED_OK);
        r=record(&b,t);assert(r->event.key[0].serial!=r->event.key[1].serial);b.now=r->event.scheduled.first;fire(&b,t);
        r->receipt.action[1].key.serial=r->receipt.action[0].key.serial;
        assert(pt_lineage_poll(q,t,&out)==PT_SCHEDULED_BACKEND&&h.held);close_all(q,&b,&h,1);}
}
static void partial_batch(void)
{
    struct memory m={0};struct backend b={0};struct holder h;struct pt_lineage_output *q=open(&m,&b,1,245);
    struct pt_scheduled_batch e;struct pt_lineage_receipt out;struct record *r;uint64_t t;
    hold(&h,1);e=batch(&h,100,0,PT_SCHEDULED_TRIGGER);e.count=2;e.action[1]=e.action[0];e.action[1].slot=1;
    assert(pt_lineage_enqueue(q,&e,NULL,&h.owner,&t)==PT_SCHEDULED_OK&&pt_lineage_publish(q,t)==PT_SCHEDULED_OK);
    r=record(&b,t);b.now=r->event.scheduled.first;fire(&b,t);r->receipt.action[1]=(struct pt_lineage_action_receipt){0};
    assert(pt_lineage_poll(q,t,&out)==PT_SCHEDULED_BACKEND&&h.held);close_all(q,&b,&h,1);
}
static void known_replacement_and_stop_contradiction(void)
{
    struct memory m={0};struct backend b={0};struct holder h[3];struct pt_lineage_output *q=open(&m,&b,3,250);
    struct pt_lineage_key key;struct pt_scheduled_batch e;struct pt_lineage_receipt out;uint64_t t,replacement,c=99;unsigned i;
    for(i=0;i<3;++i)hold(h+i,i+1);
    t=trigger(q,&b,h,100,0);assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_OK);
    e=batch(h+1,200,0,PT_SCHEDULED_TRIGGER);assert(pt_lineage_enqueue(q,&e,NULL,&h[1].owner,&replacement)==PT_SCHEDULED_OK&&pt_lineage_publish(q,replacement)==PT_SCHEDULED_OK);
    e=batch(h+2,300,0,PT_SCHEDULED_CONTROL);assert(pt_lineage_enqueue(q,&e,&key,&h[2].owner,&c)==PT_SCHEDULED_INVALID&&c==99&&h[2].held);
    released(h+2,3);close_all(q,&b,h,2);
    memset(&m,0,sizeof(m));memset(&b,0,sizeof(b));q=open(&m,&b,2,251);hold(h,1);hold(h+1,2);
    t=trigger(q,&b,h,100,0);assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_OK);
    e=batch(h+1,200,0,PT_SCHEDULED_STOP);assert(pt_lineage_enqueue(q,&e,&key,&h[1].owner,&c)==PT_SCHEDULED_OK&&pt_lineage_publish(q,c)==PT_SCHEDULED_OK);
    b.now=record(&b,c)->event.scheduled.first;fire(&b,c);record(&b,c)->receipt.action[0].reader=PT_LINEAGE_ACTIVE;
    assert(pt_lineage_poll(q,c,&out)==PT_SCHEDULED_BACKEND&&h[1].held);close_all(q,&b,h,2);
}
static void global_reader_fact_without_other_retirement(void)
{
    unsigned phase;for(phase=0;phase<2;++phase){struct memory m={0};struct backend b={0};struct holder h[3];
        struct pt_lineage_output *q=open(&m,&b,3,270+phase);struct pt_lineage_key key,before;
        struct pt_scheduled_batch e;struct pt_lineage_receipt out;struct record *r;uint64_t t,c,next=99;
        unsigned i;for(i=0;i<3;++i)hold(h+i,i+1);t=trigger(q,&b,h,100,0);
        assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_OK);e=batch(h+1,200,0,PT_SCHEDULED_CONTROL);
        assert(pt_lineage_enqueue(q,&e,&key,&h[1].owner,&c)==PT_SCHEDULED_OK&&pt_lineage_publish(q,c)==PT_SCHEDULED_OK);
        b.now=record(&b,c)->event.scheduled.first;fire(&b,c);r=record(&b,c);
        r->receipt.action[0].reader=phase?PT_LINEAGE_RETIRED:PT_LINEAGE_DRAINING;
        assert(pt_lineage_poll(q,c,&out)==PT_SCHEDULED_PENDING&&pt_lineage_held(q)==2&&h[0].held&&h[1].held);
        before=key;assert(pt_lineage_reader_key(q,t,0,&key)==PT_SCHEDULED_STALE&&!memcmp(&before,&key,sizeof(key)));
        e=batch(h+2,300,0,PT_SCHEDULED_CONTROL);assert(pt_lineage_enqueue(q,&e,&key,&h[2].owner,&next)==PT_SCHEDULED_INVALID&&next==99);
        released(h+2,3);close_all(q,&b,h,2);
    }
}
static void replacement_history_after_entry_retirement(void)
{
    unsigned mode;for(mode=0;mode<4;++mode){struct memory m={0};struct backend b={0};struct holder h[3];
        struct pt_lineage_output *q=open(&m,&b,3,280+mode);struct pt_lineage_key key,outkey;
        struct pt_scheduled_batch e;struct pt_lineage_receipt out;uint64_t original,replacement,c=99;
        unsigned i,calls;void *image;for(i=0;i<3;++i)hold(h+i,i+1);original=trigger(q,&b,h,100,0);
        assert(pt_lineage_reader_key(q,original,0,&key)==PT_SCHEDULED_OK);e=batch(h+1,200,0,PT_SCHEDULED_TRIGGER);
        assert(pt_lineage_enqueue(q,&e,NULL,&h[1].owner,&replacement)==PT_SCHEDULED_OK);
        if(mode){assert(pt_lineage_publish(q,replacement)==PT_SCHEDULED_OK);
            if(mode>=2){b.now=record(&b,replacement)->event.scheduled.first;fire(&b,replacement);
                if(mode==2)assert(pt_lineage_poll(q,replacement,&out)==PT_SCHEDULED_PENDING);
                drain(&b);}
            retire(&b,replacement,0);
        }
        assert(pt_lineage_cancel(q,replacement,&out)==PT_SCHEDULED_OK&&h[1].released==1&&h[0].held&&pt_lineage_held(q)==1);
        outkey=key;e=batch(h+2,300,0,PT_SCHEDULED_CONTROL);calls=b.submissions;
        image=malloc(m.bytes);assert(image);memcpy(image,m.p,m.bytes);
        if(mode>=2){
            assert(pt_lineage_reader_key(q,original,0,&outkey)==PT_SCHEDULED_STALE&&!memcmp(&outkey,&key,sizeof(key)));
            assert(pt_lineage_enqueue(q,&e,&key,&h[2].owner,&c)==PT_SCHEDULED_INVALID&&c==99&&b.submissions==calls);
            assert(!memcmp(image,m.p,m.bytes)&&h[0].held&&h[2].held);released(h+2,3);
        }else{
            assert(pt_lineage_reader_key(q,original,0,&outkey)==PT_SCHEDULED_OK&&equal(&outkey,&key));
            assert(pt_lineage_enqueue(q,&e,&key,&h[2].owner,&c)==PT_SCHEDULED_OK&&pt_lineage_publish(q,c)==PT_SCHEDULED_OK);
        }
        free(image);drain(&b);retire(&b,original,0);assert(pt_lineage_poll(q,original,&out)==PT_SCHEDULED_OK);
        if(mode<2){retire(&b,c,0);assert(pt_lineage_poll(q,c,&out)==PT_SCHEDULED_OK&&h[2].released==1);}
        assert(pt_lineage_close(q)&&m.allocs==m.frees);
    }
}
static void grid_parity(void)
{
    uint32_t frequencies[2]={709379,715909},rates[2]={44100,48000};unsigned f,r;
    for(f=0;f<2;++f)for(r=0;r<2;++r){struct memory m={0};struct backend b={0};struct pt_lineage_backend v=api(&b);
        struct pt_allocator a={&m,allocate,deallocate};struct pt_scheduled_grid g={100,7,frequencies[f],rates[r]};
        struct pt_lineage_output *q;unsigned frame;b.now=100;b.frequency=g.frequency;b.clock=b.submit=1;
        assert(pt_lineage_open(&a,&g,300+f*2+r,&v,1,&q)==PT_SCHEDULED_OK);
        for(frame=1;frame<=4;++frame){struct holder h;struct pt_scheduled_batch e;struct pt_lineage_receipt out;uint64_t t;
            hold(&h,frame);e=batch(&h,frame,0,PT_SCHEDULED_TRIGGER);assert(pt_lineage_enqueue(q,&e,NULL,&h.owner,&t)==PT_SCHEDULED_OK&&pt_lineage_publish(q,t)==PT_SCHEDULED_OK);
            assert(record(&b,t)->event.scheduled.first==100+((uint64_t)frame*g.frequency+g.rate-1)/g.rate);
            assert(record(&b,t)->event.scheduled.last==100+((uint64_t)(frame+1)*g.frequency+g.rate-1)/g.rate);
            b.now=record(&b,t)->event.scheduled.last-1;fire(&b,t);assert(pt_lineage_poll(q,t,&out)==PT_SCHEDULED_PENDING);
            drain(&b);retire(&b,t,0);assert(pt_lineage_poll(q,t,&out)==PT_SCHEDULED_OK&&h.released==1);
        }
        assert(pt_lineage_close(q)&&m.allocs==m.frees);
    }
}
int main(void)
{
    observed_lifecycle();cancellation_history();exact_target_and_atomic_effects();stale_before_publish();receipt_failures();
    aliases_and_keys();pressure_and_uncertainty();open_and_clock();session_and_allocated_alias();
    contradictions_and_mutable_observation();partial_batch();known_replacement_and_stop_contradiction();global_reader_fact_without_other_retirement();replacement_history_after_entry_retirement();grid_parity();
    puts("SCHEDULED LINEAGE PASS: typed actual activation, exact reader targets, independent retirement and bounded pressure; software contract only, no hardware timing proof");
    return 0;
}
