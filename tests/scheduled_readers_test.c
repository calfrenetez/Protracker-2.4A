#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Include once for bounded rollover/address-reuse white-box cases. The Python
 * recipe does not separately compile scheduled_readers.c. */
#include "../src/core/scheduled_readers.c"
static struct pt_readers_output reused_queue;static unsigned reuse_queue;
struct memory {void *p;unsigned allocs,frees,fail,reentry,nested;};
static void *allocate(void *v,size_t n)
{struct memory *m=v;if(m->fail)return NULL;m->p=reuse_queue?(assert(n==sizeof(reused_queue)),(void *)&reused_queue):malloc(n);assert(m->p);++m->allocs;return m->p;}
static void deallocate(void *v,void *p)
{struct memory *m=v;assert(p==m->p);if(m->reentry){m->reentry=0;m->nested=(unsigned)pt_readers_close(p);}++m->frees;if(!reuse_queue)free(p);m->p=NULL;}
struct holder {
    struct pt_readers_control control;unsigned held,valid,releases,terminals,terminal_valid;
    struct pt_readers_output *reenter;unsigned reentry;enum pt_scheduled_result nested;
};
union storage {
    uint64_t align;int32_t master[512];uint8_t bytes[2048];
    struct pt_readers_command_receipt command;struct pt_readers_reader_receipt reader;struct pt_readers_key key;
};
static int owner_current(void *v,uint64_t token,uint64_t generation)
{
    struct holder *h=v;
    if(h->reentry){struct pt_readers_key key;h->reentry=0;h->nested=pt_readers_reader_key(h->reenter,1,0,&key);}
    return h->held&&h->valid&&h->control.token==token&&generation==7;
}
static void owner_terminal(void *v,uint64_t token,int valid)
{struct holder *h=v;assert(h->held&&token==h->control.token&&!h->terminals);++h->terminals;h->terminal_valid=(unsigned)valid;}
static void owner_release(void *v,uint64_t token)
{struct holder *h=v;assert(h->held&&token==h->control.token&&h->terminals==1);h->held=0;++h->releases;}
static void hold(struct holder *h,uint64_t token)
{memset(h,0,sizeof(*h));h->held=h->valid=1;h->control=(struct pt_readers_control){h,sizeof(*h),token,owner_current,owner_release,owner_terminal};}
struct model_reader {
    const struct pt_readers_domain *borrowed;struct pt_readers_key key;struct holder *owner;
    struct pt_readers_reader_receipt receipt;enum pt_readers_reply reply;
    const uint8_t *data;unsigned adopted;
};
struct model_command {
    const struct pt_readers_event *borrowed;struct pt_readers_event event;
    struct pt_readers_command_receipt receipt;enum pt_readers_reply reply;unsigned fired;
};
struct backend {
    uint64_t now;uint32_t frequency;int clock,submit;unsigned submissions,reads,cpolls,rpolls,cancels,effects;
    unsigned commands,readers;struct model_command command[64];struct model_reader reader[16];
    struct holder *holders[16];unsigned holders_count;
    struct pt_readers_output *reenter;unsigned reentry;enum pt_scheduled_result nested;
};
struct fixture {
    struct memory memory;struct backend backend;struct holder command[32],reader[12];
    union storage storage[12];struct pt_scheduled_span spans[12][2];struct pt_readers_owner owner[12];
    struct pt_readers_output *q;
};
static struct fixture f;
static struct model_command *mc(struct backend *b,uint64_t ticket)
{unsigned i;for(i=0;i<b->commands;++i)if(b->command[i].event.scheduled.ticket==ticket)return b->command+i;assert(0);return NULL;}
static struct model_reader *mr(struct backend *b,const struct pt_readers_key *key)
{unsigned i;for(i=0;i<b->readers;++i)if(equal_key(&b->reader[i].key,key))return b->reader+i;assert(0);return NULL;}
static struct holder *mapped(struct backend *b,uint64_t token)
{unsigned i;for(i=0;i<b->holders_count;++i)if(b->holders[i]->control.token==token)return b->holders[i];assert(0);return NULL;}
static int read_clock(void *v,uint64_t *tick,uint32_t *hz)
{struct backend *b=v;++b->reads;*tick=b->now;*hz=b->frequency;return b->clock;}
static void command_envelope_model(struct model_command *m,struct holder *h)
{
    const struct pt_readers_event *e=&m->event;struct pt_readers_command_receipt *r=&m->receipt;
    memset(r,0,sizeof(*r));r->domain=PT_READERS_COMMAND_DOMAIN;r->origin=PT_READERS_BACKEND_ACTUAL;r->queue=e->queue;r->session=e->session;
    r->generation=e->scheduled.batch.generation;r->ticket=e->scheduled.ticket;r->owner=e->command_owner;
    r->count=e->scheduled.batch.count;r->event=m->borrowed;r->context=h;r->context_bytes=sizeof(*h);
}
static int actual_condition(struct backend *b,const struct pt_readers_event *e)
{
    unsigned i,j,k;
    for(i=0;i<e->scheduled.batch.count;++i){
        const struct pt_readers_domain *d=e->reader[i];struct holder *h=mapped(b,d->key.owner);
        if(!h->held||!h->valid||h->control.token!=d->key.owner||d->key.generation!=7)return 0;
        if(e->scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER){
            struct model_reader *r=mr(b,&d->key);
            if(!r->borrowed||!r->adopted||r->receipt.state!=PT_READERS_ACTIVE||!equal_key(&r->key,&d->key))return 0;
            /* A previous queued replacement/STOP must also close this original. */
            for(j=0;j<b->commands;++j)if(b->command[j].borrowed&&!b->command[j].fired&&b->command[j].event.scheduled.batch.frame<e->scheduled.batch.frame)
                for(k=0;k<b->command[j].event.scheduled.batch.count;++k){const struct pt_scheduled_action *a=b->command[j].event.scheduled.batch.action+k;
                    if(a->slot==d->key.slot&&(a->kind==PT_SCHEDULED_TRIGGER||a->kind==PT_SCHEDULED_STOP))return 0;}
            for(j=0;j<b->readers;++j)if(b->reader[j].adopted&&b->reader[j].key.slot==d->key.slot&&b->reader[j].key.trigger>d->key.trigger)return 0;
        }
    }
    return 1;
}
static int submit(void *v,const struct pt_readers_event *e)
{
    struct backend *b=v;struct model_command *m;unsigned i;++b->submissions;
    assert(b->now<e->scheduled.first);
    if(b->reentry){b->reentry=0;b->nested=pt_readers_publish(b->reenter,e->scheduled.ticket);}
    if(!b->submit||!actual_condition(b,e))return 0;
    assert(b->commands<64);m=b->command+b->commands++;memset(m,0,sizeof(*m));m->borrowed=e;m->event=*e;
    command_envelope_model(m,mapped(b,e->command_owner));
    for(i=0;i<e->scheduled.batch.count;++i){const struct pt_readers_domain *d=e->reader[i];
        m->receipt.action[i].key=d->key;
        if(e->scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER){
            struct model_reader *r;assert(b->readers<16);r=b->reader+b->readers++;memset(r,0,sizeof(*r));r->borrowed=d;r->key=d->key;r->owner=mapped(b,d->key.owner);r->data=e->scheduled.batch.action[i].data;
            r->receipt=(struct pt_readers_reader_receipt){PT_READERS_READER_DOMAIN,d->key,d,r->owner,sizeof(*r->owner),PT_READERS_RESERVED,PT_READERS_UNADOPTED,0,0};
        }else{m->receipt.action[i].reader=PT_READERS_ACTIVE;m->receipt.action[i].adoption=PT_READERS_ADOPTED;}
    }
    return b->submit;
}
static enum pt_readers_reply poll_command(void *v,uint64_t t,struct pt_readers_command_receipt *out)
{struct backend *b=v;struct model_command *m=mc(b,t);++b->cpolls;*out=m->receipt;if(m->reply==PT_READERS_COMMAND_DETACHED)m->borrowed=NULL;return m->reply;}
static enum pt_readers_reply cancel_command(void *v,uint64_t t,struct pt_readers_command_receipt *out)
{struct backend *b=v;++b->cancels;return poll_command(v,t,out);}
static enum pt_readers_reply poll_reader(void *v,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{struct backend *b=v;struct model_reader *r=mr(b,&d->key);++b->rpolls;*out=r->receipt;if(r->reply==PT_READERS_READER_RETIRED)r->borrowed=NULL;return r->reply;}
static enum pt_readers_reply cancel_reader(void *v,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{struct backend *b=v;++b->cancels;return poll_reader(v,d,out);}
static struct pt_readers_backend api(struct backend *b)
{return (struct pt_readers_backend){b,sizeof(*b),{7,8,4},PT_READERS_VERSION,3,8,read_clock,submit,poll_command,cancel_command,poll_reader,cancel_reader};}
static void setup_grid(unsigned commands,unsigned readers,uint64_t session,uint32_t frequency,uint32_t rate)
{
    struct pt_allocator a;struct pt_scheduled_grid g={100,7,frequency,rate};struct pt_readers_backend b;unsigned i,j;
    memset(&f,0,sizeof(f));a=(struct pt_allocator){&f.memory,allocate,deallocate};b=api(&f.backend);
    f.backend.now=100;f.backend.frequency=g.frequency;f.backend.clock=f.backend.submit=1;
    for(i=0;i<32;++i)hold(f.command+i,100+i);
    for(i=0;i<12;++i){hold(f.reader+i,1000+i);f.backend.holders[f.backend.holders_count++]=f.reader+i;
        for(j=0;j<512;++j)f.storage[i].master[j]=(int32_t)(0x123401U+j);
        f.spans[i][0]=(struct pt_scheduled_span){&f.storage[i],sizeof(f.storage[i])};f.owner[i]=(struct pt_readers_owner){f.reader[i].control,f.spans[i],1};}
    /* Map only command holders used by these finite cases. */
    assert(pt_readers_open(&a,&g,session,&b,commands,readers,&f.q)==PT_SCHEDULED_OK);
}
static void setup(unsigned commands,unsigned readers,uint64_t session)
{setup_grid(commands,readers,session,709379,48000);}
static void map_command(unsigned i)
{assert(f.backend.holders_count<16);f.backend.holders[f.backend.holders_count++]=f.command+i;}
static struct pt_scheduled_batch batch(unsigned r,uint64_t frame,unsigned slot,enum pt_scheduled_kind kind)
{struct pt_scheduled_batch b;memset(&b,0,sizeof(b));b.generation=7;b.frame=frame;b.count=1;
 b.action[0]=(struct pt_scheduled_action){kind,slot,kind==PT_SCHEDULED_TRIGGER?f.storage[r].bytes:NULL,kind==PT_SCHEDULED_TRIGGER?16:0,kind==PT_SCHEDULED_STOP?0:124,kind==PT_SCHEDULED_STOP?0:64};return b;}
static uint64_t enqueue(unsigned c,unsigned r,uint64_t frame,unsigned slot,enum pt_scheduled_kind kind,const struct pt_readers_key *key)
{struct pt_scheduled_batch b=batch(r,frame,slot,kind);uint64_t t=0;enum pt_scheduled_result result=pt_readers_enqueue(f.q,&b,key,&f.command[c].control,kind==PT_SCHEDULED_TRIGGER?f.owner+r:NULL,&t);if(result!=PT_SCHEDULED_OK)fprintf(stderr,"enqueue c%u r%u frame%llu kind%d result%d failed%u held%u/%u\n",c,r,(unsigned long long)frame,kind,result,f.q->failed,f.q->command_count,f.q->reader_count);assert(result==PT_SCHEDULED_OK);return t;}
static void fire(uint64_t t,int adopt)
{
    struct model_command *m=mc(&f.backend,t);unsigned i;assert(!m->fired);m->fired=1;m->reply=PT_READERS_OBSERVATION;
    if(f.backend.now<m->event.scheduled.first||f.backend.now>=m->event.scheduled.last||!actual_condition(&f.backend,&m->event)){
        for(i=0;i<m->event.scheduled.batch.count;++i){m->receipt.action[i].command=PT_READERS_FAILED;m->receipt.action[i].reader=PT_READERS_RETIRED;}
        return;
    }
    for(i=0;i<m->event.scheduled.batch.count;++i){const struct pt_scheduled_action *a=m->event.scheduled.batch.action+i;struct pt_readers_action_receipt *r=m->receipt.action+i;struct model_reader *p=mr(&f.backend,&r->key);
        ++f.backend.effects;r->command=PT_READERS_ISSUED;r->observed=r->issued=f.backend.now;
        if(a->kind==PT_SCHEDULED_TRIGGER){p->adopted=(unsigned)adopt;p->receipt.adoption=adopt?PT_READERS_ADOPTED:PT_READERS_UNADOPTED;p->receipt.state=adopt?PT_READERS_ACTIVE:PT_READERS_RESERVED;p->receipt.observed=p->receipt.issued=adopt?f.backend.now:0;}
        if(a->kind==PT_SCHEDULED_STOP)p->receipt.state=PT_READERS_DRAINING;
        r->reader=p->receipt.state;r->adoption=p->receipt.adoption;
    }
}
static void detach(uint64_t t)
{struct model_command *m=mc(&f.backend,t);m->reply=PT_READERS_COMMAND_DETACHED;}
static void retire(const struct pt_readers_key *key)
{struct model_reader *r=mr(&f.backend,key);r->receipt.state=PT_READERS_RETIRED;r->data=NULL;r->reply=PT_READERS_READER_RETIRED;}
static uint64_t activate(unsigned c,unsigned r,uint64_t frame,unsigned slot,struct pt_readers_key *key,int detached)
{
    uint64_t t;struct pt_readers_command_receipt out;map_command(c);t=enqueue(c,r,frame,slot,PT_SCHEDULED_TRIGGER,NULL);
    assert(pt_readers_publish(f.q,t)==PT_SCHEDULED_OK);f.backend.now=mc(&f.backend,t)->event.scheduled.first;fire(t,1);
    if(detached)detach(t);
    assert(pt_readers_poll_command(f.q,t,&out)==(detached?PT_SCHEDULED_OK:PT_SCHEDULED_PENDING));
    assert(pt_readers_reader_key(f.q,t,0,key)==PT_SCHEDULED_OK);return t;
}
static void finish(void)
{
    struct pt_readers_command_receipt c;struct pt_readers_reader_receipt r;unsigned i;
    for(i=0;i<f.backend.readers;++i)if(f.backend.reader[i].borrowed){struct model_reader *m=f.backend.reader+i;retire(&m->key);(void)pt_readers_poll_reader(f.q,m->key.trigger,m->key.action,&r);}
    for(i=0;i<f.backend.commands;++i)if(f.backend.command[i].borrowed){struct model_command *m=f.backend.command+i;unsigned j;
        for(j=0;j<m->receipt.count;++j){struct pt_readers_action_receipt *a=m->receipt.action+j;
            if(!m->fired){a->command=PT_READERS_CANCELLED_BEFORE;a->reader=m->event.scheduled.batch.action[j].kind==PT_SCHEDULED_TRIGGER?PT_READERS_NONE:PT_READERS_RETIRED;a->adoption=m->event.scheduled.batch.action[j].kind==PT_SCHEDULED_TRIGGER?PT_READERS_UNADOPTED:PT_READERS_ADOPTED;}
            else a->reader=PT_READERS_RETIRED;}
        detach(m->event.scheduled.ticket);(void)pt_readers_poll_command(f.q,m->event.scheduled.ticket,&c);}
    (void)pt_readers_stop(f.q);assert(!pt_readers_commands_held(f.q)&&!pt_readers_readers_held(f.q)&&pt_readers_close(f.q));assert(f.memory.allocs==1&&f.memory.frees==1);
}
static void repeated_controls(void)
{
    struct pt_readers_key key,again;struct pt_readers_command_receipt out;struct pt_readers_reader_receipt rr;
    uint64_t trigger,t;unsigned i;union storage before;
    setup(2,2,23);trigger=activate(0,0,100,0,&key,1);before=f.storage[0];
    assert(!pt_readers_commands_held(f.q)&&pt_readers_readers_held(f.q)==1&&f.reader[0].held);
    for(i=0;i<20;++i){/* Reuse exactly one genuine released small command holder. */
        hold(&f.command[1],101);if(i==0)map_command(1);t=enqueue(1,0,200+100*i,0,PT_SCHEDULED_CONTROL,&key);
        assert(pt_readers_publish(f.q,t)==PT_SCHEDULED_OK);f.backend.now=mc(&f.backend,t)->event.scheduled.first;fire(t,1);detach(t);
        assert(pt_readers_poll_command(f.q,t,&out)==PT_SCHEDULED_OK&&out.origin==PT_READERS_BACKEND_ACTUAL&&f.command[1].releases==1);
        assert(pt_readers_reader_key(f.q,trigger,0,&again)==PT_SCHEDULED_OK&&equal_key(&again,&key));
        assert(pt_readers_reader_key(f.q,t,0,&again)==PT_SCHEDULED_INVALID);
        assert(pt_readers_poll_command(f.q,trigger,&out)==PT_SCHEDULED_INVALID&&f.reader[0].held&&!f.reader[0].releases);
    }
    assert(f.backend.effects==21&&!memcmp(&before,&f.storage[0],sizeof(before)));
    retire(&key);assert(pt_readers_poll_reader(f.q,trigger,0,&rr)==PT_SCHEDULED_OK&&f.reader[0].releases==1);finish();
}
static void retirement_orders(void)
{
    struct pt_readers_key key;struct pt_readers_command_receipt out;struct pt_readers_reader_receipt r;uint64_t t,c;unsigned order;
    for(order=0;order<2;++order){setup(2,1,24);t=activate(0,0,100,0,&key,0);map_command(1);c=enqueue(1,0,200,0,PT_SCHEDULED_CONTROL,&key);
        assert(pt_readers_publish(f.q,c)==PT_SCHEDULED_OK);f.backend.now=mc(&f.backend,c)->event.scheduled.first;fire(c,1);
        if(!order){retire(&key);assert(pt_readers_poll_reader(f.q,t,0,&r)==PT_SCHEDULED_OK&&f.reader[0].held&&pt_readers_readers_held(f.q)==1);}
        detach(c);if(!order)mc(&f.backend,c)->receipt.action[0].reader=PT_READERS_RETIRED;
        assert(pt_readers_poll_command(f.q,c,&out)==PT_SCHEDULED_OK&&f.reader[0].held);
        detach(t);if(!order)mc(&f.backend,t)->receipt.action[0].reader=PT_READERS_RETIRED;
        assert(pt_readers_poll_command(f.q,t,&out)==PT_SCHEDULED_OK&&f.reader[0].held==(order!=0));
        if(order){retire(&key);assert(pt_readers_poll_reader(f.q,t,0,&r)==PT_SCHEDULED_OK);}
        assert(f.reader[0].releases==1&&f.reader[0].terminals==1);finish();}
}
static void unadopted_and_uncertain(void)
{
    struct pt_readers_key key,sentinel;struct pt_readers_command_receipt out;struct pt_readers_reader_receipt rr;uint64_t t;unsigned uncertain;
    for(uncertain=0;uncertain<2;++uncertain){setup(1,1,25);map_command(0);t=enqueue(0,0,100,0,PT_SCHEDULED_TRIGGER,NULL);
        f.backend.submit=uncertain?-1:1;assert(pt_readers_publish(f.q,t)==(uncertain?PT_SCHEDULED_BACKEND:PT_SCHEDULED_OK));
        f.backend.now=mc(&f.backend,t)->event.scheduled.first;fire(t,0);detach(t);memset(&out,0x33,sizeof(out));
        assert(pt_readers_poll_command(f.q,t,&out)==(uncertain?PT_SCHEDULED_BACKEND:PT_SCHEDULED_OK));
        key=mr(&f.backend,&mc(&f.backend,t)->event.reader[0]->key)->key;memset(&sentinel,0x55,sizeof(sentinel));
        assert(pt_readers_reader_key(f.q,t,0,&sentinel)!=PT_SCHEDULED_OK&&f.reader[0].held&&!f.command[0].held);
        retire(&key);assert(pt_readers_poll_reader(f.q,t,0,&rr)==(uncertain?PT_SCHEDULED_BACKEND:PT_SCHEDULED_OK)&&f.reader[0].releases==1);finish();}
}
static void stop_and_local_cancel(void)
{
    struct pt_readers_key key,sentinel;struct pt_readers_command_receipt out;struct pt_scheduled_batch b;uint64_t t,c,s;union storage before;
    setup(2,2,26);t=activate(0,0,100,0,&key,1);before=f.storage[0];
    c=enqueue(1,0,200,0,PT_SCHEDULED_CONTROL,&key);assert(pt_readers_cancel_command(f.q,c,&out)==PT_SCHEDULED_OK&&!f.backend.cancels&&out.origin==PT_READERS_LOCAL_UNSUBMITTED&&out.action[0].command==PT_READERS_CANCELLED_BEFORE&&out.action[0].reader==PT_READERS_STATE_UNKNOWN&&equal_key(&out.action[0].key,&key)&&!out.event&&!out.context&&!out.context_bytes&&!out.action[0].observed&&!out.action[0].issued);
    assert(pt_readers_reader_key(f.q,t,0,&sentinel)==PT_SCHEDULED_OK&&f.reader[0].held&&!memcmp(&before,&f.storage[0],sizeof(before)));
    hold(&f.command[1],101);map_command(1);s=enqueue(1,0,300,0,PT_SCHEDULED_STOP,&key);
    b=batch(0,400,0,PT_SCHEDULED_CONTROL);c=77;assert(pt_readers_enqueue(f.q,&b,&key,&f.command[2].control,NULL,&c)==PT_SCHEDULED_INVALID&&c==77);
    assert(pt_readers_publish(f.q,s)==PT_SCHEDULED_OK);f.backend.now=mc(&f.backend,s)->event.scheduled.first;fire(s,1);detach(s);
    assert(pt_readers_poll_command(f.q,s,&out)==PT_SCHEDULED_OK&&f.reader[0].held);
    sentinel=key;assert(pt_readers_reader_key(f.q,t,0,&sentinel)==PT_SCHEDULED_STALE&&equal_key(&sentinel,&key));finish();
    setup(1,1,27);c=enqueue(0,0,100,0,PT_SCHEDULED_TRIGGER,NULL);before=f.storage[0];
    assert(pt_readers_cancel_command(f.q,c,&out)==PT_SCHEDULED_OK&&f.command[0].releases==1&&f.reader[0].releases==1&&!f.backend.commands&&!memcmp(&before,&f.storage[0],sizeof(before)));finish();
}
static void malformed_proofs(void)
{
    struct pt_readers_key key;struct pt_readers_command_receipt c,unchanged;struct pt_readers_reader_receipt r,unchanged_r;uint64_t t;unsigned mode;
    for(mode=0;mode<6;++mode){setup(1,1,28);t=activate(0,0,100,0,&key,0);memset(&c,0x44,sizeof(c));unchanged=c;detach(t);
        if(mode==0)mc(&f.backend,t)->receipt.domain=PT_READERS_READER_DOMAIN;
        if(mode==1)mc(&f.backend,t)->receipt.owner++;
        if(mode==2)mc(&f.backend,t)->receipt.action[0].key.serial++;
        if(mode==3)mc(&f.backend,t)->receipt.action[0].issued=mc(&f.backend,t)->event.scheduled.last;
        if(mode==4)mc(&f.backend,t)->receipt.action[0].adoption=PT_READERS_UNADOPTED;
        if(mode==5)mc(&f.backend,t)->receipt.origin=PT_READERS_LOCAL_UNSUBMITTED;
        assert(pt_readers_poll_command(f.q,t,&c)==PT_SCHEDULED_BACKEND&&!memcmp(&c,&unchanged,sizeof(c))&&f.reader[0].held);
        assert(f.command[0].releases==(mode>=2&&mode<5));
        /* An independently exact proof can still retire the appropriate domain. */
        if(mode<2||mode==5){mc(&f.backend,t)->borrowed=&command(f.q,t)->event;command_envelope_model(mc(&f.backend,t),f.command);mc(&f.backend,t)->receipt.action[0]=(struct pt_readers_action_receipt){PT_READERS_ISSUED,PT_READERS_ACTIVE,PT_READERS_ADOPTED,key,reader(f.q,t,0)->observed,reader(f.q,t,0)->issued};
            assert(pt_readers_poll_command(f.q,t,&c)==PT_SCHEDULED_BACKEND&&f.command[0].releases==1);}
        retire(&key);assert(pt_readers_poll_reader(f.q,t,0,&r)==PT_SCHEDULED_BACKEND&&f.reader[0].releases==1);finish();}
    for(mode=0;mode<4;++mode){setup(1,1,29);t=activate(0,0,100,0,&key,1);retire(&key);memset(&r,0x22,sizeof(r));unchanged_r=r;
        if(mode==0)mr(&f.backend,&key)->receipt.domain=PT_READERS_COMMAND_DOMAIN;
        if(mode==1)mr(&f.backend,&key)->receipt.key.owner++;
        if(mode==2)mr(&f.backend,&key)->receipt.state=PT_READERS_ACTIVE;
        if(mode==3)mr(&f.backend,&key)->receipt.issued=reader(f.q,t,0)->last;
        assert(pt_readers_poll_reader(f.q,t,0,&r)==PT_SCHEDULED_BACKEND&&!memcmp(&r,&unchanged_r,sizeof(r)));
        assert(f.reader[0].releases==(mode>=2));
        if(mode<2){struct model_reader *m=mr(&f.backend,&key);m->borrowed=&reader(f.q,t,0)->domain;m->receipt.domain=PT_READERS_READER_DOMAIN;m->receipt.key=key;
            assert(pt_readers_poll_reader(f.q,t,0,&r)==PT_SCHEDULED_BACKEND&&f.reader[0].releases==1);}
        assert(f.reader[0].terminal_valid==(mode<2));finish();}
}
static void replacement_monotone(void)
{
    struct pt_readers_key old,newkey,unchanged;struct pt_readers_command_receipt c;struct pt_readers_reader_receipt r;
    uint64_t t,n;unsigned via_reader;
    for(via_reader=0;via_reader<2;++via_reader){setup(2,2,30);t=activate(0,0,100,0,&old,1);
        /* Same cache bytes, genuine independently retained holder. */
        f.owner[1].spans=f.spans[0];map_command(1);{struct pt_scheduled_batch b=batch(1,200,0,PT_SCHEDULED_TRIGGER);b.action[0].data=f.storage[0].bytes;assert(pt_readers_enqueue(f.q,&b,NULL,&f.command[1].control,f.owner+1,&n)==PT_SCHEDULED_OK);}assert(pt_readers_publish(f.q,n)==PT_SCHEDULED_OK);
        f.backend.now=mc(&f.backend,n)->event.scheduled.first;fire(n,1);newkey=mc(&f.backend,n)->event.reader[0]->key;
        if(via_reader){retire(&newkey);assert(pt_readers_poll_reader(f.q,n,0,&r)==PT_SCHEDULED_OK&&f.reader[1].held);}
        else assert(pt_readers_poll_command(f.q,n,&c)==PT_SCHEDULED_PENDING);
        assert(!active(f.q,reader(f.q,t,0),UINT64_MAX,0));
        detach(n);if(via_reader)mc(&f.backend,n)->receipt.action[0].reader=PT_READERS_RETIRED;
        assert(pt_readers_poll_command(f.q,n,&c)==PT_SCHEDULED_OK);
        if(!via_reader){retire(&newkey);assert(pt_readers_poll_reader(f.q,n,0,&r)==PT_SCHEDULED_OK);}
        unchanged=old;assert(pt_readers_reader_key(f.q,t,0,&unchanged)==PT_SCHEDULED_STALE&&equal_key(&old,&unchanged)&&f.reader[0].held);
        finish();}
}
static void pressure_reuse_and_rollover(void)
{
    struct pt_readers_key key,old;struct pt_readers_command_receipt c;struct pt_readers_reader_receipt r;struct pt_scheduled_batch b;
    uint64_t t,n,out;struct pt_readers_output *qimage;unsigned i;
    setup(2,1,31);t=activate(0,0,100,0,&key,1);old=key;
    b=batch(1,200,1,PT_SCHEDULED_TRIGGER);out=88;assert(pt_readers_enqueue(f.q,&b,NULL,&f.command[1].control,f.owner+1,&out)==PT_SCHEDULED_CAPACITY&&out==88);
    retire(&key);assert(pt_readers_poll_reader(f.q,t,0,&r)==PT_SCHEDULED_OK);hold(&f.reader[0],1000);hold(&f.command[0],100);
    n=activate(0,0,300,0,&key,1);assert(n!=t&&key.serial!=old.serial&&key.owner==old.owner);
    b=batch(0,400,0,PT_SCHEDULED_CONTROL);out=88;assert(pt_readers_enqueue(f.q,&b,&old,&f.command[1].control,NULL,&out)==PT_SCHEDULED_INVALID&&out==88);
    /* Old bound receipt cannot detach an event at the recycled same address. */
    hold(&f.command[1],101);map_command(1);out=enqueue(1,0,500,0,PT_SCHEDULED_CONTROL,&key);assert(pt_readers_publish(f.q,out)==PT_SCHEDULED_OK);
    f.backend.now=mc(&f.backend,out)->event.scheduled.first;fire(out,1);detach(out);mc(&f.backend,out)->receipt=mc(&f.backend,t)->receipt;
    assert(pt_readers_poll_command(f.q,out,&c)==PT_SCHEDULED_BACKEND&&f.command[1].held);mc(&f.backend,out)->borrowed=&command(f.q,out)->event;command_envelope_model(mc(&f.backend,out),f.command+1);
    mc(&f.backend,out)->receipt.action[0]=(struct pt_readers_action_receipt){PT_READERS_ISSUED,PT_READERS_ACTIVE,PT_READERS_ADOPTED,key,f.backend.now,f.backend.now};mc(&f.backend,out)->borrowed=&command(f.q,out)->event;
    assert(pt_readers_poll_command(f.q,out,&c)==PT_SCHEDULED_BACKEND&&!f.command[1].held);finish();
    setup(1,1,32);qimage=malloc(sizeof(*qimage));assert(qimage);
    for(i=0;i<2;++i){f.q->tickets=i?0:UINT64_MAX;f.q->serial=i?UINT64_MAX:0;*qimage=*f.q;b=batch(0,100,0,PT_SCHEDULED_TRIGGER);out=88;
        assert(pt_readers_enqueue(f.q,&b,NULL,&f.command[0].control,f.owner,&out)==PT_SCHEDULED_CAPACITY&&out==88&&!memcmp(qimage,f.q,sizeof(*qimage)));}
    free(qimage);finish();
}
static void output_and_refusal_guards(void)
{
    struct pt_readers_key key;struct pt_readers_command_receipt c;struct pt_readers_reader_receipt r;struct pt_scheduled_batch b;
    union storage before;struct holder hbefore;struct pt_readers_output *qbefore;struct backend *bbefore;
    void *aliases[6];size_t sizes[6];uint64_t t,out;unsigned i,reads,polls;
    setup(2,2,33);t=activate(0,0,100,0,&key,1);qbefore=malloc(sizeof(*qbefore));bbefore=malloc(sizeof(*bbefore));assert(qbefore&&bbefore);
    aliases[0]=&f.storage[0].key;aliases[1]=f.storage[0].bytes+sizeof(f.storage[0])-1;aliases[2]=f.reader;aliases[3]=f.q;aliases[4]=&f.backend;aliases[5]=(void *)(UINTPTR_MAX-2);
    sizes[0]=sizes[1]=sizeof(key);sizes[2]=sizeof(key);sizes[3]=sizeof(key);sizes[4]=sizeof(key);sizes[5]=sizeof(key);
    for(i=0;i<6;++i){before=f.storage[0];hbefore=f.reader[0];*qbefore=*f.q;*bbefore=f.backend;polls=f.backend.rpolls;
        assert(!output_apart(f.q,aliases[i],sizes[i]));assert(pt_readers_reader_key(f.q,t,0,aliases[i])==PT_SCHEDULED_INVALID);
        assert(!memcmp(&before,&f.storage[0],sizeof(before))&&!memcmp(&hbefore,f.reader,sizeof(hbefore))&&!memcmp(qbefore,f.q,sizeof(*qbefore))&&!memcmp(bbefore,&f.backend,sizeof(*bbefore))&&f.backend.rpolls==polls);}
    before=f.storage[0];polls=f.backend.rpolls;assert(pt_readers_poll_reader(f.q,t,0,&f.storage[0].reader)==PT_SCHEDULED_INVALID&&f.backend.rpolls==polls&&!memcmp(&before,&f.storage[0],sizeof(before)));
    out=enqueue(1,0,200,0,PT_SCHEDULED_CONTROL,&key);reads=f.backend.reads;polls=f.backend.cpolls;hbefore=f.command[1];*qbefore=*f.q;
    assert(pt_readers_cancel_command(f.q,out,(struct pt_readers_command_receipt *)f.command[1].control.context)==PT_SCHEDULED_INVALID&&f.backend.cpolls==polls&&f.backend.reads==reads&&!memcmp(&hbefore,f.command+1,sizeof(hbefore))&&!memcmp(qbefore,f.q,sizeof(*qbefore)));
    assert(pt_readers_cancel_command(f.q,out,&c)==PT_SCHEDULED_OK);hold(&f.command[1],101);b=batch(0,300,0,PT_SCHEDULED_CONTROL);before=f.storage[0];out=99;
    assert(pt_readers_enqueue(f.q,&b,&key,&f.command[1].control,NULL,(uint64_t *)&f.storage[0].bytes[2040])==PT_SCHEDULED_INVALID&&!memcmp(&before,&f.storage[0],sizeof(before)));
    /* Genuine active source version/current is required before publication. */
    out=enqueue(1,0,300,0,PT_SCHEDULED_CONTROL,&key);f.reader[0].valid=0;reads=f.backend.reads;assert(pt_readers_publish(f.q,out)==PT_SCHEDULED_STALE&&f.backend.reads==reads);f.reader[0].valid=1;
    assert(pt_readers_cancel_command(f.q,out,&c)==PT_SCHEDULED_OK);retire(&key);assert(pt_readers_poll_reader(f.q,t,0,&r)==PT_SCHEDULED_OK);free(qbefore);free(bbefore);finish();
}
static void clock_atomic_and_reentry(void)
{
    struct pt_readers_key key;struct pt_readers_command_receipt c;struct pt_readers_reader_receipt r;uint64_t t;unsigned mode,effects;
    for(mode=0;mode<4;++mode){setup(1,1,34);map_command(0);t=enqueue(0,0,100,0,PT_SCHEDULED_TRIGGER,NULL);
        if(mode==0)f.backend.frequency++;
        if(mode==1)f.backend.now=command(f.q,t)->event.scheduled.first;
        if(mode==2)f.command[0].valid=0;
        if(mode==3){f.backend.reenter=f.q;f.backend.reentry=1;}
        assert(pt_readers_publish(f.q,t)==(mode==0?PT_SCHEDULED_CLOCK:mode==1?PT_SCHEDULED_LATE:mode==2?PT_SCHEDULED_STALE:PT_SCHEDULED_BACKEND));
        if(mode==3)assert(f.backend.nested==PT_SCHEDULED_BACKEND&&f.reader[0].held);
        finish();}
    setup(2,2,35);t=activate(0,0,100,0,&key,1);map_command(1);t=enqueue(1,0,200,0,PT_SCHEDULED_CONTROL,&key);assert(pt_readers_publish(f.q,t)==PT_SCHEDULED_OK);
    f.reader[0].valid=0;effects=f.backend.effects;f.backend.now=mc(&f.backend,t)->event.scheduled.first;fire(t,1);assert(f.backend.effects==effects);detach(t);
    assert(pt_readers_poll_command(f.q,t,&c)==PT_SCHEDULED_BACKEND);f.reader[0].valid=1;retire(&key);(void)pt_readers_poll_reader(f.q,key.trigger,0,&r);finish();
    setup(1,1,36);f.reader[0].reentry=1;f.reader[0].reenter=f.q;t=88;{struct pt_scheduled_batch b=batch(0,100,0,PT_SCHEDULED_TRIGGER);assert(pt_readers_enqueue(f.q,&b,NULL,&f.command[0].control,f.owner,&t)==PT_SCHEDULED_BACKEND&&t==88);}
    assert(f.reader[0].nested==PT_SCHEDULED_BACKEND&&!pt_readers_readers_held(f.q));finish();
}

static void incoming_resource_control_overlap(void)
{
    struct pt_readers_key key;struct pt_scheduled_batch b;struct pt_readers_output *before;
    struct holder reader_before,command_before;union storage master_before;uint64_t ticket;
    unsigned polls,reads;
    setup(2,2,37);(void)activate(0,0,100,0,&key,1);
    before=malloc(sizeof(*before));assert(before);*before=*f.q;
    reader_before=f.reader[0];command_before=f.command[1];master_before=f.storage[1];
    f.spans[1][1]=(struct pt_scheduled_span){(const uint8_t *)f.reader+sizeof(f.reader[0])-1,1};f.owner[1].count=2;
    b=batch(1,200,1,PT_SCHEDULED_TRIGGER);ticket=99;polls=f.backend.rpolls;reads=f.backend.reads;
    assert(pt_readers_enqueue(f.q,&b,NULL,&f.command[1].control,f.owner+1,&ticket)==PT_SCHEDULED_INVALID&&ticket==99);
    assert(!memcmp(before,f.q,sizeof(*before))&&!memcmp(&reader_before,f.reader,sizeof(reader_before))&&!memcmp(&command_before,f.command+1,sizeof(command_before))&&!memcmp(&master_before,f.storage+1,sizeof(master_before))&&polls==f.backend.rpolls&&reads==f.backend.reads);
    free(before);finish();
}


static void forged_keys_and_whole_batch(void)
{
    struct pt_readers_key keys[2],bad;struct pt_scheduled_batch b;struct pt_readers_command_receipt c;
    struct pt_readers_reader_receipt r;struct pt_readers_output *before;uint64_t t[2],ticket;unsigned i,effects;
    setup(2,2,38);t[0]=activate(0,0,100,0,keys,1);t[1]=activate(1,1,200,1,keys+1,1);
    before=malloc(sizeof(*before));assert(before);
    for(i=0;i<8;++i){bad=keys[0];if(i==0)bad.queue=NULL;if(i==1)bad.session++;if(i==2)bad.generation++;if(i==3)bad.trigger++;if(i==4)bad.owner++;if(i==5)bad.serial++;if(i==6)bad.action++;if(i==7)bad.slot++;
        *before=*f.q;b=batch(0,300,0,PT_SCHEDULED_CONTROL);ticket=99;
        assert(pt_readers_enqueue(f.q,&b,&bad,&f.command[2].control,NULL,&ticket)==PT_SCHEDULED_INVALID&&ticket==99&&!memcmp(before,f.q,sizeof(*before)));}
    free(before);map_command(2);b=batch(0,300,0,PT_SCHEDULED_CONTROL);b.count=2;b.action[1]=b.action[0];b.action[1].slot=1;
    assert(pt_readers_enqueue(f.q,&b,keys,&f.command[2].control,NULL,&ticket)==PT_SCHEDULED_OK&&pt_readers_publish(f.q,ticket)==PT_SCHEDULED_OK);
    /* Simulated actual activation compares the whole batch before ANY effect,
     * even if the stale second reader's action is visited last. */
    f.reader[1].valid=0;effects=f.backend.effects;f.backend.now=mc(&f.backend,ticket)->event.scheduled.first;fire(ticket,1);assert(f.backend.effects==effects);detach(ticket);
    assert(pt_readers_poll_command(f.q,ticket,&c)==PT_SCHEDULED_BACKEND&&f.command[2].terminal_valid==0);
    f.reader[1].valid=1;for(i=0;i<2;++i){retire(keys+i);(void)pt_readers_poll_reader(f.q,t[i],0,&r);}finish();
}
static void independent_pressure_and_pending_outputs(void)
{
    struct pt_readers_key key;struct pt_readers_command_receipt c,cbefore;struct pt_readers_reader_receipt r,rbefore;
    struct pt_scheduled_batch b;struct pt_readers_output *before;uint64_t t,a,d,out;unsigned polls;
    setup(2,1,39);t=activate(0,0,100,0,&key,1);
    a=enqueue(1,0,200,0,PT_SCHEDULED_CONTROL,&key);d=enqueue(2,0,300,0,PT_SCHEDULED_CONTROL,&key);
    before=malloc(sizeof(*before));assert(before);*before=*f.q;b=batch(0,400,0,PT_SCHEDULED_CONTROL);out=99;
    assert(pt_readers_enqueue(f.q,&b,&key,&f.command[3].control,NULL,&out)==PT_SCHEDULED_CAPACITY&&out==99&&!memcmp(before,f.q,sizeof(*before)));free(before);
    map_command(1);f.backend.submit=0;assert(pt_readers_publish(f.q,a)==PT_SCHEDULED_PENDING&&f.command[1].held&&!command(f.q,a)->published);f.backend.submit=1;
    assert(pt_readers_publish(f.q,a)==PT_SCHEDULED_OK);memset(&c,0x44,sizeof(c));cbefore=c;polls=f.backend.cpolls;
    assert(pt_readers_poll_command(f.q,a,&c)==PT_SCHEDULED_PENDING&&!memcmp(&c,&cbefore,sizeof(c))&&f.backend.cpolls==polls+1);
    memset(&r,0x44,sizeof(r));rbefore=r;polls=f.backend.rpolls;assert(pt_readers_poll_reader(f.q,t,0,&r)==PT_SCHEDULED_PENDING&&!memcmp(&r,&rbefore,sizeof(r))&&f.backend.rpolls==polls+1);
    assert(pt_readers_cancel_command(f.q,d,&c)==PT_SCHEDULED_OK&&f.reader[0].held);finish();
}


static void reused_queue_address_fresh_session(void)
{
    struct pt_readers_key old,key;struct pt_scheduled_batch b;uint64_t ticket;const struct pt_readers_output *address;
    reuse_queue=1;setup(1,1,40);(void)activate(0,0,100,0,&old,1);address=f.q;finish();
    setup(1,1,41);(void)activate(0,0,100,0,&key,1);assert(f.q==address&&key.queue==old.queue&&key.trigger==old.trigger&&key.serial==old.serial&&key.owner==old.owner&&key.session!=old.session);
    b=batch(0,200,0,PT_SCHEDULED_CONTROL);ticket=99;assert(pt_readers_enqueue(f.q,&b,&old,&f.command[1].control,NULL,&ticket)==PT_SCHEDULED_INVALID&&ticket==99);finish();reuse_queue=0;
}


static void sticky_reader_invalid_and_close_reentry(void)
{
    struct pt_readers_key key;struct pt_readers_command_receipt c;struct pt_readers_reader_receipt r,unchanged;
    uint64_t t,control;unsigned order;
    for(order=0;order<2;++order){setup(2,1,42);t=activate(0,0,100,0,&key,0);map_command(1);control=enqueue(1,0,200,0,PT_SCHEDULED_CONTROL,&key);
        assert(pt_readers_publish(f.q,control)==PT_SCHEDULED_OK);f.backend.now=mc(&f.backend,control)->event.scheduled.first;fire(control,1);
        if(order){detach(t);assert(pt_readers_poll_command(f.q,t,&c)==PT_SCHEDULED_OK);}
        retire(&key);mr(&f.backend,&key)->receipt.issued=reader(f.q,t,0)->last;memset(&r,0x44,sizeof(r));unchanged=r;
        assert(pt_readers_poll_reader(f.q,t,0,&r)==PT_SCHEDULED_BACKEND&&!memcmp(&r,&unchanged,sizeof(r))&&f.reader[0].held&&!reader(f.q,t,0)->valid);
        mr(&f.backend,&key)->receipt.issued=reader(f.q,t,0)->issued;mr(&f.backend,&key)->borrowed=&reader(f.q,t,0)->domain;
        assert(pt_readers_poll_reader(f.q,t,0,&r)==PT_SCHEDULED_BACKEND&&!memcmp(&r,&unchanged,sizeof(r))&&f.reader[0].held&&!reader(f.q,t,0)->valid);
        detach(control);mc(&f.backend,control)->receipt.action[0].reader=PT_READERS_RETIRED;assert(pt_readers_poll_command(f.q,control,&c)==PT_SCHEDULED_BACKEND);
        if(!order){detach(t);mc(&f.backend,t)->receipt.action[0].reader=PT_READERS_RETIRED;assert(pt_readers_poll_command(f.q,t,&c)==PT_SCHEDULED_BACKEND);}
        assert(f.reader[0].releases==1&&f.reader[0].terminals==1&&f.reader[0].terminal_valid==0);finish();}
    setup(1,1,43);f.memory.reentry=1;assert(pt_readers_close(f.q)&&f.memory.nested==0&&f.memory.frees==1&&f.memory.p==NULL);
}


static void grid_and_open_contracts(void)
{
    uint32_t frequencies[2]={709379,715909},rates[2]={44100,48000};unsigned i,j;
    struct pt_readers_key key;uint64_t ticket,first,last;struct pt_allocator a;
    struct pt_scheduled_grid grid={100,7,709379,48000};struct pt_readers_backend b;struct pt_readers_output *out;
    for(i=0;i<2;++i)for(j=0;j<2;++j){setup_grid(1,1,50+i*2+j,frequencies[i],rates[j]);ticket=activate(0,0,100,0,&key,1);
        assert(pt_elapsed_clock_deadline(&f.q->clock,100,&first)==PT_ELAPSED_OK&&pt_elapsed_clock_deadline(&f.q->clock,101,&last)==PT_ELAPSED_OK);
        assert(mc(&f.backend,ticket)->event.scheduled.first==first&&mc(&f.backend,ticket)->event.scheduled.last==last&&reader(f.q,ticket,0)->issued==first);finish();}
    memset(&f,0,sizeof(f));a=(struct pt_allocator){&f.memory,allocate,deallocate};b=api(&f.backend);
    for(i=0;i<5;++i){struct pt_readers_backend bad=b;out=(void *)(uintptr_t)9;if(i==0)bad.version++;if(i==1)bad.reference_flags=1;if(i==2)bad.reference_flags=7;if(i==3)bad.caps.flags=3;if(i==4)bad.maximum_readers=0;
        assert(pt_readers_open(&a,&grid,60,&bad,1,1,&out)==PT_SCHEDULED_UNSUPPORTED&&out==(void *)(uintptr_t)9&&!f.memory.allocs);}
    f.memory.fail=1;out=(void *)(uintptr_t)9;assert(pt_readers_open(&a,&grid,60,&b,1,1,&out)==PT_SCHEDULED_CAPACITY&&out==(void *)(uintptr_t)9&&!f.memory.allocs);
}

int main(void)
{
    repeated_controls();retirement_orders();unadopted_and_uncertain();stop_and_local_cancel();malformed_proofs();
    replacement_monotone();pressure_reuse_and_rollover();output_and_refusal_guards();incoming_resource_control_overlap();forged_keys_and_whole_batch();independent_pressure_and_pending_outputs();reused_queue_address_fresh_session();sticky_reader_invalid_and_close_reentry();grid_and_open_contracts();clock_atomic_and_reentry();
    puts("SCHEDULED READERS PASS: independent command detach and persistent reader retirement; 20 controls with command capacity2");return 0;
}
