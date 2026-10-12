#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../src/editor/sampler_paula_segment_readers.h"
#include "../src/editor/sampler_paula_readers.h"
#include "../src/editor/sampler_internal.h"
#define RATE 48000U
#define STEP (((uint64_t)8000U<<32)/RATE)
static unsigned cases;
enum {M_SUBMIT=1,M_CLOCK,M_POLL_COMMAND,M_CANCEL_COMMAND,M_POLL_READER,M_CANCEL_READER};
struct memory {unsigned live,calls;size_t bytes;unsigned mutate_at,mutate_field;
    struct pt_sampler_paula *mutate_bridge;struct pt_paula_segment_readers_config *mutate_config;};
static void *allocate(void *v,size_t n)
{struct memory *m=v;void *p=malloc(n);++m->calls;if(p)++m->live;
 if(p&&m->calls==m->mutate_at){if(!m->mutate_field)m->mutate_bridge->sampler=NULL;
    else if(m->mutate_field==1)m->mutate_bridge->project=NULL;else ++m->mutate_config->frame[1];}
 return p;}
static void release(void *v,void *p)
{struct memory *m=v;if(p){assert(m->live);--m->live;free(p);}}
static void *chip_allocate(void *v,size_t n)
{struct memory *m=v;void *p=allocate(v,n);if(p)m->bytes+=n;return p;}
static void chip_release(void *v,void *p,size_t n)
{struct memory *m=v;assert(m->bytes>=n);m->bytes-=n;release(v,p);}
static int key_equal(const struct pt_readers_key *a,const struct pt_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->slot==b->slot;}
struct model_command {
    const struct pt_readers_event *borrowed;const struct pt_paula_segment_plan *plan_borrowed;
    struct pt_readers_event event;struct pt_paula_segment_plan plan;
    struct pt_readers_command_receipt receipt;enum pt_readers_reply reply;unsigned fired;
};
struct model {
    uint64_t now;uint32_t frequency;int raw;unsigned count,effects,controls,stops,adopt,mutate,malformed;
    struct pt_paula_segment_readers *reenter;unsigned nested,reentry;
    struct model_command command[4];const struct pt_readers_domain *reader;
    struct pt_readers_key key;struct pt_readers_reader_receipt receipt;enum pt_readers_reply reply;
    const uint8_t *initial,*pending,*superseded;const struct pt_sample_version *expected[3];
    unsigned span_count;struct pt_scheduled_span spans[PT_SCHEDULED_SPANS];
    struct pt_readers_domain domain_snapshot;unsigned corruption,corruption_phase;
};
struct fixture {
    struct memory fast,chip;struct pt_allocator allocator;struct pt_document document;
    struct pt_sampler sampler;struct pt_sampler_paula bridge;struct pt_sample_version *pin[3];struct pt_pcm pcm[3];
    int32_t original[3][32];struct pt_pattern_history history;struct pt_pattern_command command[4];struct pt_event_change changes[4];
    struct model model;struct pt_paula_segment_backend backend;struct pt_paula_segment_readers_config config;
    struct pt_paula_segment_readers *pool;
};
static struct model_command *find(struct model *m,uint64_t ticket)
{unsigned i;for(i=0;i<m->count;++i)if(m->command[i].event.scheduled.ticket==ticket)return m->command+i;assert(0);return NULL;}
/* Deliberate violations of actually loaned public descriptor/opaque-binding
 * storage. No genuine queue/private core cast, ownership-counter change, fake
 * ACTIVE state or fabricated retirement is used. Product must restore identity
 * before core resumes, and every real reference still needs independent proof. */
static int corrupt(struct model *m,unsigned phase,const struct pt_readers_event *event,const struct pt_readers_domain *domain)
{
    struct pt_readers_event *e;struct pt_readers_domain *d;struct pt_scheduled_span *spans;unsigned mode;
    if(!m->corruption||m->corruption_phase!=phase)return 0;
    mode=m->corruption;m->corruption=0;
    e=(struct pt_readers_event *)(event?event:m->command[0].borrowed);
    d=(struct pt_readers_domain *)(domain?domain:m->reader);assert(e&&d&&d->count);
    spans=(struct pt_scheduled_span *)d->spans;
    if(mode==1)++e->scheduled.ticket;
    else if(mode==2)++e->scheduled.batch.frame;
    else if(mode==3)e->scheduled.batch.count=PT_READERS_ACTIONS+1;
    else if(mode==4)e->scheduled.batch.action[0].kind=PT_SCHEDULED_STOP;
    else if(mode==5)e->reader[0]=(const struct pt_readers_domain *)e;
    else if(mode==6)e->binding.context=NULL;
    else if(mode==7)e->queue=NULL;
    else if(mode==8)d->spans=NULL;
    else if(mode==9)d->count=UINT32_MAX;
    else if(mode==10)++d->key.serial;
    else if(mode==11)spans[0].data=NULL;
    else if(mode==12)spans[0].bytes=SIZE_MAX;
    else if(mode==13)memset(e->binding.context,0,e->binding.context_bytes);
    else if(mode==14)memset(d->binding.context,0,d->binding.context_bytes);
    else if(mode==15||mode==16){void *poison=(void *)(uintptr_t)1;
        void *holder=mode==15?e->binding.context:d->binding.context;
        size_t extent=mode==15?e->binding.context_bytes:d->binding.context_bytes;
        assert(extent>=sizeof(poison));memcpy(holder,&poison,sizeof(poison));}
    else assert(0);
    return 1;
}
static int clock_read(void *v,uint64_t *tick,uint32_t *frequency)
{struct model *m=v;*tick=m->now;*frequency=m->frequency;corrupt(m,M_CLOCK,NULL,NULL);return 1;}
static int span_covers(const struct pt_readers_domain *d,const void *data,size_t n)
{unsigned i;uintptr_t p=(uintptr_t)data;for(i=0;i<d->count;++i){uintptr_t q=(uintptr_t)d->spans[i].data;
 if(p>=q&&p-q<=d->spans[i].bytes&&n<=d->spans[i].bytes-(p-q))return 1;}return 0;}
static int submit_plan(void *v,const struct pt_readers_event *event,const struct pt_paula_segment_plan *plan)
{
    struct model *m=v;struct model_command *c;const struct pt_readers_domain *d=event->reader[0];unsigned i;int changed;
    assert(m->count<4&&event->scheduled.batch.count==1&&event->scheduled.batch.frame==plan->frame);
    assert(m->now<event->scheduled.first&&event->binding.context&&event->binding.context_bytes);
    assert(plan->slot==d->key.slot&&event->queue==d->key.queue&&event->session==d->key.session);
    for(i=0;i<plan->source_count;++i){const struct pt_paula_segment_source *s=plan->source+i;
        assert(s->sample<3&&s->version==m->expected[s->sample]);
        assert(span_covers(d,s->master.data,s->master.capacity*sizeof(int32_t))&&span_covers(d,s->cache_data,s->cache_bytes));}
    if(plan->kind==PT_PAULA_SEGMENT_INITIAL){assert(!m->reader);m->reader=d;m->key=d->key;
        assert(d->count<=PT_SCHEDULED_SPANS);m->span_count=d->count;memcpy(m->spans,d->spans,d->count*sizeof(*d->spans));m->domain_snapshot=*d;
        m->receipt=(struct pt_readers_reader_receipt){PT_READERS_READER_DOMAIN,d->key,d,d->binding.context,d->binding.context_bytes,PT_READERS_RESERVED,PT_READERS_UNADOPTED,0,0};
    }else{assert(m->reader==d&&key_equal(&m->key,&d->key));
        assert(m->receipt.state==PT_READERS_ACTIVE&&m->receipt.adoption==PT_READERS_ADOPTED);}
    c=m->command+m->count++;memset(c,0,sizeof(*c));c->borrowed=event;c->plan_borrowed=plan;c->event=*event;c->plan=*plan;
    c->receipt=(struct pt_readers_command_receipt){PT_READERS_COMMAND_DOMAIN,PT_READERS_BACKEND_ACTUAL,event->queue,event->session,
        event->scheduled.batch.generation,event->scheduled.ticket,event->command_owner,1,event,event->binding.context,event->binding.context_bytes,{{0}}};
    c->receipt.action[0].key=d->key;
    if(plan->kind!=PT_PAULA_SEGMENT_INITIAL){c->receipt.action[0].reader=PT_READERS_ACTIVE;c->receipt.action[0].adoption=PT_READERS_ADOPTED;}
    if(m->reentry){m->reentry=0;m->nested=(unsigned)pt_paula_segment_readers_close(m->reenter);}
    if(m->mutate){m->mutate=0;((struct pt_paula_segment_plan *)plan)->frame++;}
    changed=corrupt(m,M_SUBMIT,event,d);
    if(!m->raw&&!changed){c->borrowed=NULL;c->plan_borrowed=NULL;m->reader=NULL;} /* Real clean refusal retains no references. */
    return m->raw;
}
static enum pt_readers_reply command_reply(void *v,uint64_t ticket,struct pt_readers_command_receipt *out,unsigned phase)
{
    struct model *m=v;struct model_command *c=find(m,ticket);*out=c->receipt;
    if(m->malformed==1){++out->context_bytes;return PT_READERS_COMMAND_DETACHED;}
    corrupt(m,phase,c->borrowed,m->reader);
    if(c->reply==PT_READERS_COMMAND_DETACHED){c->borrowed=NULL;c->plan_borrowed=NULL;}return c->reply;
}
static enum pt_readers_reply poll_command(void *v,uint64_t ticket,struct pt_readers_command_receipt *out)
{return command_reply(v,ticket,out,M_POLL_COMMAND);}
static enum pt_readers_reply cancel_command(void *v,uint64_t ticket,struct pt_readers_command_receipt *out)
{return command_reply(v,ticket,out,M_CANCEL_COMMAND);}
static enum pt_readers_reply reader_reply(void *v,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out,unsigned phase)
{
    struct model *m=v;assert(key_equal(&d->key,&m->key));*out=m->receipt;
    if(m->malformed==2){++out->key.serial;return PT_READERS_READER_RETIRED;}
    corrupt(m,phase,NULL,d);
    if(m->reply==PT_READERS_READER_RETIRED){assert(!m->initial&&!m->pending&&!m->superseded);m->reader=NULL;}
    return m->reply;
}
static enum pt_readers_reply poll_reader(void *v,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{return reader_reply(v,d,out,M_POLL_READER);}
static enum pt_readers_reply cancel_reader(void *v,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{return reader_reply(v,d,out,M_CANCEL_READER);}
static void init(struct fixture *f)
{
    unsigned i,j;memset(f,0,sizeof(*f));f->allocator=(struct pt_allocator){&f->fast,allocate,release};
    pt_document_init(&f->document,&f->allocator);assert(pt_document_new(&f->document,4,SIZE_MAX)==PT_PROJECT_OK);
    for(j=0;j<3;++j){for(i=0;i<32;++i)f->original[j][i]=(int32_t)i-16+(int32_t)j;
        f->document.project.samples[j].pcm=(struct pt_pcm){f->original[j],32,32,8000,1,8};}
    pt_sampler_init(&f->sampler,&f->allocator,1024*1024);
    assert(pt_pattern_history_init(&f->history,&f->document.project,f->command,4,f->changes,4)==PT_EDIT_OK);
    for(i=0;i<3;++i)assert(pt_sampler_pin(&f->sampler,&f->document.project,i,f->sampler.generation,f->pcm+i,f->pin+i)==PT_EDIT_OK);
    assert(pt_sampler_paula_bind(&f->bridge,&f->sampler,&f->document.project,&f->chip,chip_allocate,chip_release,96));
    f->config=(struct pt_paula_segment_readers_config){0,3,{{0,f->pin[0]},{1,f->pin[1]},{2,f->pin[2]}},
        {100,7,709379,RATE},13,{960,1920,2880,3840},{3546895,124,65535},256*1024};
    f->model.now=100;f->model.frequency=709379;f->model.raw=1;f->model.adopt=1;
    for(i=0;i<3;++i)f->model.expected[i]=f->pin[i];
    f->backend=(struct pt_paula_segment_backend){{&f->model,sizeof(f->model),{7,4,1},PT_READERS_VERSION,PT_READERS_REQUIRED,1,
        clock_read,NULL,poll_command,cancel_command,poll_reader,cancel_reader},PT_PAULA_SEGMENT_BACKEND_VERSION,PT_PAULA_SEGMENT_BACKEND_REQUIRED,submit_plan};
}
static void open_fixture(struct fixture *f)
{assert(pt_paula_segment_readers_open(&f->allocator,&f->bridge,&f->config,&f->backend,&f->pool)==PT_PAULA_SEGMENT_READERS_OK&&f->pool);f->model.reenter=f->pool;}
static void prepare(struct fixture *f)
{unsigned ready=0,n=0;enum pt_paula_segment_readers_result result;do{result=pt_paula_segment_readers_step(f->pool,&ready);
 assert(result==PT_PAULA_SEGMENT_READERS_PENDING||result==PT_PAULA_SEGMENT_READERS_OK);assert(++n<100);}while(!ready);
 assert(result==PT_PAULA_SEGMENT_READERS_OK&&f->chip.bytes==96);}
static unsigned pinned(struct fixture *f)
{unsigned i,n=0;for(i=0;i<PT_CACHE_SLOTS;++i)n+=f->bridge.cache.entry[i].pins;return n;}
static void storage(struct fixture *f)
{unsigned i;for(i=0;i<3;++i)assert(!memcmp(f->pcm[i].data,f->original[i],sizeof(f->original[i])));}
static void close_fixture(struct fixture *f)
{
    unsigned i;assert(!f->pool);assert(pt_sampler_paula_close(&f->bridge)&&!f->chip.live&&!f->chip.bytes);
    storage(f);pt_pattern_history_release(&f->history);pt_sampler_release(&f->sampler);
    for(i=0;i<3;++i){assert(!memcmp(f->pcm[i].data,f->original[i],sizeof(f->original[i])));pt_sampler_unpin(f->pin[i]);}
    pt_document_release(&f->document);assert(!f->fast.live);
}
static struct pt_render_action segment(struct fixture *f)
{struct pt_render_action a;memset(&a,0,sizeof(a));a.kind=PT_RENDER_SEGMENT;a.gain[0]=65536;
 assert(pt_voice_init_segment(&a.voice,&f->document.project.samples[0].pcm,2,18,4,12,STEP,0)==PT_PCM_OK);return a;}
static unsigned enqueue(struct fixture *f,unsigned original,struct pt_render_action *a,const struct pt_readers_key *key)
{unsigned index=99;uint64_t ticket=0;assert(pt_paula_segment_readers_begin(f->pool,f->config.frame[original],a,key,&index)==PT_PAULA_SEGMENT_READERS_OK&&index==original);
 assert(pt_paula_segment_readers_enqueue(f->pool,index,&ticket)==PT_SCHEDULED_OK&&ticket);return index;}
static void fire(struct fixture *f,unsigned index)
{
    struct model *m=&f->model;struct model_command *c=m->command+index;struct pt_readers_action_receipt *r=c->receipt.action;
    assert(c->borrowed&&c->plan_borrowed&&!c->fired&&c->event.scheduled.batch.frame==f->config.frame[index]);
    m->now=c->event.scheduled.first;assert(m->now<c->event.scheduled.last);c->fired=1;c->reply=PT_READERS_OBSERVATION;
    r->command=PT_READERS_ISSUED;r->observed=r->issued=m->now;++m->effects;
    if(!index){m->receipt.state=m->adopt?PT_READERS_ACTIVE:PT_READERS_RESERVED;m->receipt.adoption=m->adopt?PT_READERS_ADOPTED:PT_READERS_UNADOPTED;
        m->receipt.observed=m->receipt.issued=m->adopt?m->now:0;m->initial=c->event.scheduled.batch.action[0].data;
        m->pending=c->plan.source[0].cache_data+c->plan.repeat.byte_offset;
    }else if(index==3){++m->stops;m->receipt.state=PT_READERS_DRAINING;}
    else{++m->controls;m->superseded=m->pending;
        m->pending=c->plan.source[c->plan.source_count-1].cache_data+c->plan.repeat.byte_offset;
        assert(!c->plan.write_initial&&c->plan.write_repeat&&m->initial);}
    r->reader=m->receipt.state;r->adoption=m->receipt.adoption;
}
static void detach(struct fixture *f,unsigned index)
{struct model_command *c=f->model.command+index;c->reply=PT_READERS_COMMAND_DETACHED;
 if(f->model.receipt.state==PT_READERS_RETIRED)c->receipt.action[0].reader=PT_READERS_RETIRED;}
static void genuine_quiet(struct fixture *f)
{struct model *m=&f->model;m->initial=m->pending=m->superseded=NULL;m->receipt.state=PT_READERS_RETIRED;m->reply=PT_READERS_READER_RETIRED;}
static void collect_reader(struct fixture *f)
{struct pt_readers_reader_receipt rr;enum pt_scheduled_result r=pt_paula_segment_readers_poll_reader(f->pool,0,&rr);assert(r==PT_SCHEDULED_OK||r==PT_SCHEDULED_BACKEND);}
static void collect_command(struct fixture *f,unsigned index)
{struct pt_readers_command_receipt cr;enum pt_scheduled_result r=pt_paula_segment_readers_poll_command(f->pool,index,0,&cr);assert(r==PT_SCHEDULED_OK||r==PT_SCHEDULED_BACKEND);}
static void trigger(struct fixture *f,struct pt_render_action *a,struct pt_readers_key *key,int detach_now)
{struct pt_readers_command_receipt cr;struct pt_readers_key before;memset(&before,0x39,sizeof(before));*key=before;
 enqueue(f,0,a,NULL);assert(pt_paula_segment_readers_key(f->pool,key)!=PT_SCHEDULED_OK&&!memcmp(key,&before,sizeof(before)));
 assert(pt_paula_segment_readers_publish(f->pool,0)==PT_SCHEDULED_OK);fire(f,0);if(detach_now)detach(f,0);
 assert(pt_paula_segment_readers_poll_command(f->pool,0,0,&cr)==(detach_now?PT_SCHEDULED_OK:PT_SCHEDULED_PENDING));
 assert(pt_paula_segment_readers_key(f->pool,key)==PT_SCHEDULED_OK&&key_equal(key,&f->model.key));}
static void positive(unsigned order)
{
    struct fixture f;struct pt_render_action a;struct pt_readers_key key,again;struct pt_readers_command_receipt cr;
    struct pt_readers_reader_receipt rr;uint64_t phase;unsigned i;init(&f);open_fixture(&f);prepare(&f);a=segment(&f);trigger(&f,&a,&key,1);
    assert(pinned(&f)==3&&!pt_paula_segment_readers_close(f.pool));
    for(i=1;i<3;++i){assert(pt_voice_advance(&a.voice,1,3)==PT_PCM_OK);phase=a.voice.phase;assert(phase&UINT32_MAX);
        assert(pt_voice_set_repeat_source(&a.voice,&f.document.project.samples[i].pcm,6+i*2,14+i*2)==PT_PCM_OK);a.kind=PT_RENDER_REPEAT;
        enqueue(&f,i,&a,&key);assert(pt_paula_segment_readers_publish(f.pool,i)==PT_SCHEDULED_OK);
        assert(f.model.command[i].plan.phase_q32==phase&&f.model.command[i].plan.source[1].version==f.pin[i]);
        assert(f.model.span_count==f.model.reader->count&&!memcmp(f.model.spans,f.model.reader->spans,f.model.span_count*sizeof(*f.model.spans)));
        fire(&f,i);detach(&f,i);assert(pt_paula_segment_readers_poll_command(f.pool,i,0,&cr)==PT_SCHEDULED_OK);
        assert(pt_paula_segment_readers_key(f.pool,&again)==PT_SCHEDULED_OK&&key_equal(&again,&key));assert(pinned(&f)==3);storage(&f);
    }
    assert(f.model.superseded==f.model.command[1].plan.source[1].cache_data+f.model.command[1].plan.repeat.byte_offset);
    a.kind=PT_RENDER_STOP;enqueue(&f,3,&a,&key);again=key;
    assert(pt_paula_segment_readers_key(f.pool,&again)==PT_SCHEDULED_STALE&&key_equal(&again,&key));
    assert(pt_paula_segment_readers_publish(f.pool,3)==PT_SCHEDULED_OK);fire(&f,3);
    assert(pt_paula_segment_readers_poll_command(f.pool,3,0,&cr)==PT_SCHEDULED_PENDING&&pinned(&f)==3);
    assert(!pt_paula_segment_readers_close(f.pool)&&f.model.initial&&f.model.pending&&f.model.superseded);
    if(!order){genuine_quiet(&f);assert(pt_paula_segment_readers_poll_reader(f.pool,0,&rr)==PT_SCHEDULED_OK);assert(pinned(&f)==3);}
    detach(&f,3);assert(pt_paula_segment_readers_poll_command(f.pool,3,0,&cr)==PT_SCHEDULED_OK);
    if(order){assert(pinned(&f)==3);genuine_quiet(&f);assert(pt_paula_segment_readers_poll_reader(f.pool,0,&rr)==PT_SCHEDULED_OK);}
    assert(!pinned(&f)&&f.model.effects==4&&f.model.controls==2&&f.model.stops==1&&!f.model.reader);
    assert(pt_paula_segment_readers_close(f.pool));f.pool=NULL;close_fixture(&f);++cases;
}
static int legacy_submit(void *v,const struct pt_readers_event *e)
{(void)v;(void)e;assert(0);return 0;}
static void open_refusers(void)
{
    struct fixture f;struct pt_paula_segment_readers_config c;struct pt_paula_segment_backend b;struct pt_paula_segment_readers *out;
    unsigned mode,calls;for(mode=0;mode<8;++mode){init(&f);c=f.config;b=f.backend;out=NULL;calls=f.fast.calls;
        if(mode==0)c.dependency_count=0;if(mode==1)c.dependency_count=4;if(mode==2)c.dependency[2]=c.dependency[1];
        if(mode==3)c.dependency[1].expected=f.pin[0];if(mode==4)b.readers.submit=legacy_submit;
        if(mode==5)b.plan_version=0;if(mode==6)c.frame[2]=c.frame[1];if(mode==7)c.track=16;
        assert(pt_paula_segment_readers_open(&f.allocator,&f.bridge,&c,&b,&out)!=PT_PAULA_SEGMENT_READERS_OK);
        assert(!out&&f.fast.calls==calls&&!f.chip.calls);close_fixture(&f);++cases;}
}
static void begin_refusers(void)
{
    struct fixture f;struct pt_render_action a,b;struct pt_readers_key key,changed;struct pt_render_plan plan;
    struct pt_readers_output *q=NULL;struct pt_readers_backend backend;struct pt_paula_readers_pool *pool=NULL;
    struct pt_paula_readers_command *command=NULL;struct pt_paula_readers_config config={4,1,256*1024,96,7,NULL,NULL,NULL};
    unsigned index=99,calls,mode;init(&f);open_fixture(&f);prepare(&f);a=segment(&f);calls=f.fast.calls;
    assert(pt_paula_segment_readers_begin(f.pool,961,&a,NULL,&index)==PT_PAULA_SEGMENT_READERS_INVALID&&index==99);++cases;
    b=a;b.kind=PT_RENDER_TRIGGER;assert(pt_paula_segment_readers_begin(f.pool,960,&b,NULL,&index)==PT_PAULA_SEGMENT_READERS_INVALID&&index==99);++cases;
    assert(f.fast.calls==calls);trigger(&f,&a,&key,1);assert(pt_voice_advance(&a.voice,1,3)==PT_PCM_OK);
    assert(pt_voice_set_repeat_source(&a.voice,&f.document.project.samples[1].pcm,6,14)==PT_PCM_OK);a.kind=PT_RENDER_REPEAT;
    for(mode=0;mode<4;++mode){changed=key;if(!mode)++changed.serial;if(mode==1)++changed.slot;if(mode==2)++changed.session;if(mode==3)++changed.trigger;
        assert(pt_paula_segment_readers_begin(f.pool,1920,&a,&changed,&index)==PT_PAULA_SEGMENT_READERS_STALE&&index==99);++cases;}
    b=a;b.voice.repeat_pcm=&f.document.project.samples[3].pcm;
    assert(pt_paula_segment_readers_begin(f.pool,1920,&b,&key,&index)==PT_PAULA_SEGMENT_READERS_INVALID&&index==99);++cases;
    /* The accepted old lowerer still rejects both bookkeeping action kinds. */
    backend=f.backend.readers;backend.submit=legacy_submit;
    assert(pt_readers_open(&f.allocator,&f.config.grid,97,&backend,4,1,&q)==PT_SCHEDULED_OK);
    config.chip_context=&f.chip;config.chip_allocate=chip_allocate;config.chip_release=chip_release;
    assert(pt_paula_readers_open(&f.allocator,&f.sampler,&f.document.project,&config,&pool)==PT_PAULA_READERS_OK);
    memset(&plan,0,sizeof(plan));plan.count=1;plan.action[0]=segment(&f);
    assert(pt_paula_readers_lower_begin(pool,q,960,RATE,&plan,&f.config.caps,NULL,&command)==PT_PAULA_READERS_INVALID&&!command);
    plan.action[0]=a;
    assert(pt_paula_readers_lower_begin(pool,q,1920,RATE,&plan,&f.config.caps,NULL,&command)==PT_PAULA_READERS_INVALID&&!command);
    assert(pt_paula_readers_close(pool)&&pt_readers_stop(q)==PT_SCHEDULED_OK&&pt_readers_close(q));++cases;
    genuine_quiet(&f);collect_reader(&f);assert(pt_paula_segment_readers_close(f.pool));f.pool=NULL;close_fixture(&f);
}
static void prepare_cancel(void)
{
    struct fixture f;unsigned mode,ready,n;for(mode=0;mode<2;++mode){init(&f);open_fixture(&f);ready=0;
        for(n=0;n<(mode?2U:1U);++n)assert(pt_paula_segment_readers_step(f.pool,&ready)==PT_PAULA_SEGMENT_READERS_PENDING&&!ready);
        assert(!f.model.count&&pt_paula_segment_readers_close(f.pool));f.pool=NULL;assert(!pinned(&f));close_fixture(&f);++cases;}
}
static void stale_sources(void)
{
    struct fixture f;struct pt_render_action a;struct pt_readers_key key;unsigned mode,index=99,ready;
    for(mode=0;mode<2;++mode){init(&f);open_fixture(&f);if(mode){prepare(&f);a=segment(&f);trigger(&f,&a,&key,1);}else{
        ready=0;assert(pt_paula_segment_readers_step(f.pool,&ready)==PT_PAULA_SEGMENT_READERS_PENDING);}
        assert(pt_sampler_edit(&f.sampler,&f.document.project,&f.history,1,PT_PCM_REVERSE,0,32,0)==PT_EDIT_OK);
        assert(!pt_sampler_paula_sync(&f.bridge)||f.bridge.version!=1);
        if(mode){a.kind=PT_RENDER_REPEAT;assert(pt_voice_set_repeat_source(&a.voice,&f.document.project.samples[2].pcm,6,14)==PT_PCM_OK);
            assert(pt_paula_segment_readers_begin(f.pool,1920,&a,&key,&index)==PT_PAULA_SEGMENT_READERS_STALE&&index==99&&pinned(&f)==3);
            assert(!pt_paula_segment_readers_close(f.pool));genuine_quiet(&f);collect_reader(&f);
        }else assert(pt_paula_segment_readers_step(f.pool,&ready)==PT_PAULA_SEGMENT_READERS_STALE);
        assert(pt_paula_segment_readers_close(f.pool));f.pool=NULL;assert(!pinned(&f));close_fixture(&f);++cases;}
}
static void uncertain_submissions(void)
{
    struct fixture f;struct pt_render_action a;struct pt_readers_command_receipt cr;struct pt_readers_key key;unsigned mode;
    for(mode=0;mode<3;++mode){init(&f);open_fixture(&f);prepare(&f);a=segment(&f);enqueue(&f,0,&a,NULL);
        if(!mode)f.model.raw=-1;if(mode==1){f.model.raw=0;f.model.mutate=1;}if(mode==2)f.model.reentry=1;
        assert(pt_paula_segment_readers_publish(f.pool,0)==PT_SCHEDULED_BACKEND&&pinned(&f)==3);
        assert(f.model.count==1&&!f.model.effects&&!pt_paula_segment_readers_close(f.pool));
        assert(pt_paula_segment_readers_key(f.pool,&key)!=PT_SCHEDULED_OK);
        assert(f.model.command[0].plan.frame==960);if(mode==2)assert(!f.model.nested);
        /* No activation occurred. Exact independent model reference release
         * proves genuine unadopted retirement; unknown issue is never invented. */
        f.model.command[0].receipt.action[0].command=PT_READERS_CANCELLED_BEFORE;
        f.model.command[0].receipt.action[0].reader=PT_READERS_NONE;
        detach(&f,0);assert(pt_paula_segment_readers_poll_command(f.pool,0,0,&cr)==PT_SCHEDULED_BACKEND&&pinned(&f)==3);
        genuine_quiet(&f);collect_reader(&f);assert(!pinned(&f));
        assert(pt_paula_segment_readers_close(f.pool));f.pool=NULL;close_fixture(&f);++cases;}
}
static void malformed_receipts(void)
{
    struct fixture f;struct pt_render_action a;struct pt_readers_key key;struct pt_readers_command_receipt cr;struct pt_readers_reader_receipt rr;
    unsigned mode;for(mode=1;mode<3;++mode){init(&f);open_fixture(&f);prepare(&f);a=segment(&f);trigger(&f,&a,&key,0);f.model.malformed=mode;
        if(mode==1){detach(&f,0);assert(pt_paula_segment_readers_poll_command(f.pool,0,0,&cr)==PT_SCHEDULED_BACKEND);}
        else assert(pt_paula_segment_readers_poll_reader(f.pool,0,&rr)==PT_SCHEDULED_BACKEND);
        assert(pinned(&f)==3&&!pt_paula_segment_readers_close(f.pool));f.model.malformed=0;
        genuine_quiet(&f);collect_reader(&f);assert(pinned(&f)==3);detach(&f,0);collect_command(&f,0);assert(!pinned(&f));
        assert(pt_paula_segment_readers_close(f.pool));f.pool=NULL;close_fixture(&f);++cases;}
}
static void never_adopted(void)
{
    struct fixture f;struct pt_render_action a;struct pt_readers_key key;struct pt_readers_command_receipt cr;
    init(&f);open_fixture(&f);prepare(&f);a=segment(&f);enqueue(&f,0,&a,NULL);f.model.adopt=0;
    assert(pt_paula_segment_readers_publish(f.pool,0)==PT_SCHEDULED_OK);fire(&f,0);detach(&f,0);
    assert(pt_paula_segment_readers_poll_command(f.pool,0,0,&cr)==PT_SCHEDULED_OK&&pinned(&f)==3);
    assert(pt_paula_segment_readers_key(f.pool,&key)!=PT_SCHEDULED_OK&&!pt_paula_segment_readers_close(f.pool));
    genuine_quiet(&f);collect_reader(&f);assert(!pinned(&f)&&pt_paula_segment_readers_close(f.pool));f.pool=NULL;close_fixture(&f);++cases;
}
static void local_stop(void)
{
    struct fixture f;struct pt_render_action a;init(&f);open_fixture(&f);prepare(&f);a=segment(&f);enqueue(&f,0,&a,NULL);
    assert(pt_paula_segment_readers_stop(f.pool)==PT_SCHEDULED_OK&&!f.model.count&&!pinned(&f));
    assert(pt_paula_segment_readers_close(f.pool));f.pool=NULL;close_fixture(&f);++cases;
}
static void clean_refusal(void)
{
    struct fixture f;struct pt_render_action a;init(&f);open_fixture(&f);prepare(&f);a=segment(&f);enqueue(&f,0,&a,NULL);f.model.raw=0;
    assert(pt_paula_segment_readers_publish(f.pool,0)==PT_SCHEDULED_PENDING&&!f.model.effects&&!f.model.reader);
    assert(!f.model.command[0].borrowed&&!f.model.command[0].plan_borrowed&&pinned(&f)==3);
    assert(!pt_paula_segment_readers_close(f.pool));
    assert(pt_paula_segment_readers_stop(f.pool)==PT_SCHEDULED_OK&&!pinned(&f));
    assert(pt_paula_segment_readers_close(f.pool));f.pool=NULL;close_fixture(&f);++cases;
}
static void aligned_aliases(void)
{
    struct fixture f;struct pt_allocator a;struct pt_paula_segment_readers_config c;
    struct pt_paula_segment_backend b;struct pt_sampler_paula bridge;unsigned mode,calls,ready;
    struct pt_render_action action;struct pt_readers_key key;uint8_t cache[32];const uint8_t *data;
    for(mode=0;mode<4;++mode){init(&f);a=f.allocator;c=f.config;b=f.backend;bridge=f.bridge;calls=f.fast.calls;
        if(!mode)assert(pt_paula_segment_readers_open(&f.allocator,&f.bridge,&f.config,&f.backend,(struct pt_paula_segment_readers **)&f.allocator)==PT_PAULA_SEGMENT_READERS_INVALID);
        if(mode==1)assert(pt_paula_segment_readers_open(&f.allocator,&f.bridge,&f.config,&f.backend,(struct pt_paula_segment_readers **)&f.config)==PT_PAULA_SEGMENT_READERS_INVALID);
        if(mode==2)assert(pt_paula_segment_readers_open(&f.allocator,&f.bridge,&f.config,&f.backend,(struct pt_paula_segment_readers **)&f.backend)==PT_PAULA_SEGMENT_READERS_INVALID);
        if(mode==3)assert(pt_paula_segment_readers_open(&f.allocator,&f.bridge,&f.config,&f.backend,(struct pt_paula_segment_readers **)&f.bridge)==PT_PAULA_SEGMENT_READERS_INVALID);
        assert(f.fast.calls==calls&&!memcmp(&a,&f.allocator,sizeof(a))&&!memcmp(&c,&f.config,sizeof(c))&&!memcmp(&b,&f.backend,sizeof(b))&&!memcmp(&bridge,&f.bridge,sizeof(bridge)));
        close_fixture(&f);++cases;}
    init(&f);open_fixture(&f);prepare(&f);ready=0;
    assert(pt_paula_segment_readers_step(f.pool,(unsigned *)(f.pcm[0].data+4))==PT_PAULA_SEGMENT_READERS_INVALID);storage(&f);++cases;
    action=segment(&f);trigger(&f,&action,&key,1);data=f.model.command[0].plan.source[0].cache_data;memcpy(cache,data,32);
    assert(pt_voice_advance(&action.voice,1,3)==PT_PCM_OK&&pt_voice_set_repeat_source(&action.voice,&f.document.project.samples[1].pcm,6,14)==PT_PCM_OK);action.kind=PT_RENDER_REPEAT;
    assert(pt_paula_segment_readers_begin(f.pool,1920,&action,&key,(unsigned *)(data+4))==PT_PAULA_SEGMENT_READERS_INVALID);
    assert(!memcmp(cache,data,32)&&!ready&&pinned(&f)==3);++cases;
    genuine_quiet(&f);collect_reader(&f);assert(pt_paula_segment_readers_close(f.pool));f.pool=NULL;close_fixture(&f);
}
static void constructor_callback_mutation(void)
{
    struct fixture f;struct pt_sampler_paula bridge;struct pt_paula_segment_readers_config config;unsigned mode,live;
    for(mode=0;mode<3;++mode){init(&f);bridge=f.bridge;config=f.config;live=f.fast.live;
        f.fast.mutate_at=f.fast.calls+mode+1;f.fast.mutate_field=mode;f.fast.mutate_bridge=&f.bridge;f.fast.mutate_config=&f.config;
        assert(pt_paula_segment_readers_open(&f.allocator,&f.bridge,&f.config,&f.backend,&f.pool)==PT_PAULA_SEGMENT_READERS_STALE);
        assert(!f.pool&&f.fast.live==live&&!f.chip.calls);
        if(!mode)assert(!f.bridge.sampler);if(mode==1)assert(!f.bridge.project);if(mode==2)assert(f.config.frame[1]==config.frame[1]+1);
        /* Undo only this test's exact scalar injection after the real fail-closed
         * API result. No owner history, queue proof or sampler generation edit. */
        if(!mode)f.bridge.sampler=bridge.sampler;if(mode==1)f.bridge.project=bridge.project;if(mode==2)f.config.frame[1]=config.frame[1];
        f.fast.mutate_at=0;close_fixture(&f);++cases;
    }
}
static void misaligned_refusers(void)
{
    union {uint64_t alignment;unsigned char bytes[sizeof(struct pt_paula_segment_readers_config)+sizeof(struct pt_paula_segment_backend)+64];} bytes;
    struct fixture f;struct pt_paula_segment_readers *out;unsigned mode,calls;
    for(mode=0;mode<4;++mode){init(&f);out=NULL;calls=f.fast.calls;
        if(!mode)assert(pt_paula_segment_readers_open((struct pt_allocator *)(bytes.bytes+1),&f.bridge,&f.config,&f.backend,&out)==PT_PAULA_SEGMENT_READERS_INVALID);
        if(mode==1)assert(pt_paula_segment_readers_open(&f.allocator,&f.bridge,(struct pt_paula_segment_readers_config *)(bytes.bytes+1),&f.backend,&out)==PT_PAULA_SEGMENT_READERS_INVALID);
        if(mode==2)assert(pt_paula_segment_readers_open(&f.allocator,&f.bridge,&f.config,(struct pt_paula_segment_backend *)(bytes.bytes+1),&out)==PT_PAULA_SEGMENT_READERS_INVALID);
        if(mode==3)assert(pt_paula_segment_readers_open(&f.allocator,&f.bridge,&f.config,&f.backend,(struct pt_paula_segment_readers **)(bytes.bytes+1))==PT_PAULA_SEGMENT_READERS_INVALID);
        assert(!out&&f.fast.calls==calls&&!f.chip.calls);close_fixture(&f);++cases;}
}
static void original_descriptors(struct fixture *f,const struct pt_readers_event *event,const struct pt_readers_domain *domain)
{
    assert(!memcmp(event,&f->model.command[0].event,sizeof(*event)));
    assert(!memcmp(domain,&f->model.domain_snapshot,sizeof(*domain)));
    assert(domain->spans==f->model.domain_snapshot.spans&&domain->count==f->model.span_count);
    assert(!memcmp(domain->spans,f->model.spans,(size_t)f->model.span_count*sizeof(*domain->spans)));
}
static void mutation_cleanup(struct fixture *f,unsigned reader_first,unsigned unissued)
{
    struct model_command *c=&f->model.command[0];
    if(unissued){c->receipt.action[0].command=PT_READERS_CANCELLED_BEFORE;c->receipt.action[0].reader=PT_READERS_NONE;}
    if(reader_first){genuine_quiet(f);collect_reader(f);assert(pinned(f)==3);}
    detach(f,0);if(unissued)c->receipt.action[0].reader=PT_READERS_NONE;collect_command(f,0);
    if(!reader_first){assert(pinned(f)==3);genuine_quiet(f);collect_reader(f);}
    assert(!pinned(f)&&pt_paula_segment_readers_close(f->pool));f->pool=NULL;close_fixture(f);
}
static void submit_descriptor_mutations(void)
{
    struct fixture f;struct pt_render_action a;struct pt_readers_key key,before;
    const struct pt_readers_event *event;const struct pt_readers_domain *domain;unsigned mode,raw;
    for(mode=1;mode<=16;++mode)for(raw=0;raw<2;++raw){
        init(&f);open_fixture(&f);prepare(&f);a=segment(&f);enqueue(&f,0,&a,NULL);
        f.model.raw=(int)raw;f.model.corruption=mode;f.model.corruption_phase=M_SUBMIT;
        assert(pt_paula_segment_readers_publish(f.pool,0)==PT_SCHEDULED_BACKEND);
        assert(f.model.count==1&&!f.model.effects&&!f.model.corruption&&pinned(&f)==3);
        event=f.model.command[0].borrowed;domain=f.model.reader;assert(event&&domain);
        original_descriptors(&f,event,domain);
        if(mode==15||mode==16){void *restored=NULL;const void *holder=mode==15?event->binding.context:domain->binding.context;
            memcpy(&restored,holder,sizeof(restored));assert(restored==f.pool);}
        memset(&before,0x47,sizeof(before));key=before;
        assert(pt_paula_segment_readers_key(f.pool,&key)==PT_SCHEDULED_STALE&&!memcmp(&key,&before,sizeof(key)));
        assert(!pt_paula_segment_readers_close(f.pool)&&pinned(&f)==3);storage(&f);
        /* Callback raw0 is untrusted after descriptor mutation: it cannot imply
         * absence or discard even one real held sample/cache dependency. */
        mutation_cleanup(&f,(mode+raw)&1U,1);++cases;
    }
}
static void retained_callback_mutations(void)
{
    static const unsigned modes[4]={3,9,13,14};
    struct fixture f;struct pt_render_action a;struct pt_readers_key key;struct pt_readers_command_receipt cr,before;
    struct pt_readers_reader_receipt rr,old;const struct pt_readers_event *event;const struct pt_readers_domain *domain;
    unsigned phase,i;
    for(phase=M_CLOCK;phase<=M_CANCEL_READER;++phase)for(i=0;i<4;++i){
        init(&f);open_fixture(&f);prepare(&f);a=segment(&f);trigger(&f,&a,&key,0);
        event=f.model.command[0].borrowed;domain=f.model.reader;assert(event&&domain);
        f.model.corruption=modes[i];f.model.corruption_phase=phase;
        if(phase==M_CLOCK){
            assert(pt_voice_advance(&a.voice,1,3)==PT_PCM_OK&&pt_voice_set_repeat_source(&a.voice,&f.document.project.samples[1].pcm,6,14)==PT_PCM_OK);
            a.kind=PT_RENDER_REPEAT;enqueue(&f,1,&a,&key);
            assert(pt_paula_segment_readers_publish(f.pool,1)==PT_SCHEDULED_CLOCK&&f.model.count==1);
            assert(pt_paula_segment_readers_stop(f.pool)==PT_SCHEDULED_BACKEND); /* real local C disposal only */
        }else if(phase==M_POLL_COMMAND||phase==M_CANCEL_COMMAND){
            detach(&f,0);memset(&before,0x61,sizeof(before));cr=before;
            assert(pt_paula_segment_readers_poll_command(f.pool,0,phase==M_CANCEL_COMMAND,&cr)==PT_SCHEDULED_BACKEND);
            assert(!memcmp(&cr,&before,sizeof(cr))&&!f.model.command[0].borrowed);
        }else{
            genuine_quiet(&f);memset(&old,0x62,sizeof(old));rr=old;
            assert(pt_paula_segment_readers_poll_reader(f.pool,phase==M_CANCEL_READER,&rr)==PT_SCHEDULED_BACKEND);
            assert(!memcmp(&rr,&old,sizeof(rr))&&!f.model.reader);
        }
        assert(!f.model.corruption&&pinned(&f)==3&&!pt_paula_segment_readers_close(f.pool));
        original_descriptors(&f,event,domain);assert(f.model.effects==1);storage(&f);
        /* Even a discarded positive reply cannot free core ownership. A later
         * genuine exact callback supplies the proof again, independently. */
        mutation_cleanup(&f,phase&1U,0);++cases;
    }
}
static void retained_between_calls(void)
{
    struct fixture f;struct pt_render_action a;struct pt_readers_key key,old;unsigned mode;
    const struct pt_readers_event *event;const struct pt_readers_domain *domain;
    for(mode=0;mode<2;++mode){init(&f);open_fixture(&f);prepare(&f);a=segment(&f);trigger(&f,&a,&key,0);
        event=f.model.command[0].borrowed;domain=f.model.reader;
        f.model.corruption=mode?14U:3U;f.model.corruption_phase=M_CLOCK;
        assert(corrupt(&f.model,M_CLOCK,event,domain));old=key;
        assert(pt_paula_segment_readers_key(f.pool,&key)==PT_SCHEDULED_STALE&&key_equal(&key,&old));
        original_descriptors(&f,event,domain);assert(pinned(&f)==3&&!pt_paula_segment_readers_close(f.pool));
        mutation_cleanup(&f,mode,0);++cases;
    }
}
int main(void)
{
    positive(0);positive(1);open_refusers();begin_refusers();prepare_cancel();stale_sources();uncertain_submissions();malformed_receipts();never_adopted();local_stop();clean_refusal();misaligned_refusers();aligned_aliases();constructor_callback_mutation();submit_descriptor_mutations();retained_callback_mutations();retained_between_calls();
    assert(cases==97);
    puts("PAULA SEGMENT REPEAT READERS HOST FIXTURE PASS:97 cases;genuine3 current pins and selective Chip leases;immutable original reader domain;exact960-1920-2880-3840;sealed geometry-aware trigger-repeat-repeat-stop;fractional fetch phase and superseded source retained;independent command-reader retirement both orders;stale-unknown-unadopted-malformed-alias-constructor-custody refusers;raw0-raw1 event-domain-span-holder recovery;nonzero poisoned holder pool refusal;clock-submit-poll-cancel retained custody;genuine release ends anchors;legacy SEGMENT REPEAT rejection preserved;HOST_RESOURCE_MODEL_NATIVE_OUTPUT_NOT_RUN");
    return 0;
}
